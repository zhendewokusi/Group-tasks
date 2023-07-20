#include "threadpool.h"
// #include <bits/pthreadtypes.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUMBER 3

// 任务的结构体
typedef struct Task {
    void (*function)(void* arg);
    void* arg;
} Task;

// 线程池的结构体
struct ThreadPool {
    Task* TaskQueue;    // 任务队列
    int QueueCapacity;  // 任务队列的容量
    int QueueSize;      // 当前任务个数
    int Queuefront;     // 任务队头  取任务
    int Queueend;       // 任务队尾  放任务

    pthread_t* ThreadIDs;     // 工作线程（多个）
    pthread_t MonitorThread;  // 监视线程
    int MaxThreadNum;         // 最大线程数量
    int MinThreadNum;         // 最小线程数量

    int BusyThreadNum;  // 工作线程数量
    int ExitThreadNum;  // 需要销毁的线程数量
    int LiveThreadNum;  // 存活的线程数量

    pthread_mutex_t MutexThreadPool;  // 互斥锁锁整个线程池
    pthread_mutex_t MutexBusyThread;  // 互斥锁锁运行的线程
    pthread_cond_t Full;              // 队列是否满了
    pthread_cond_t Empty;             // 队列是否空着

    bool ShutDown;  // 是否销毁线程池
};

ThreadPool* ThreadPoolCreat(int max, int min, int queuesize) {
    ThreadPool* pool = (ThreadPool*)malloc(sizeof(ThreadPool));
    do {
        if (pool == NULL) {
            printf("malloc pool fail...\n");
            break;
        }

        pool->ThreadIDs = (pthread_t*)malloc(sizeof(pthread_t) * max);
        if (pool->ThreadIDs == NULL) {
            printf("malloc ThreadIDs fail...\n");
            break;
        }
        memset(pool->ThreadIDs, 0, sizeof(pthread_t) * max);
        pool->MaxThreadNum = max;
        pool->MinThreadNum = min;
        pool->ExitThreadNum = 0;
        pool->BusyThreadNum = 0;
        pool->LiveThreadNum = min;

        pool->TaskQueue = (Task*)malloc(sizeof(Task) * queuesize);
        if (pool->TaskQueue == NULL) {
            printf("malloc taskqueue fail...\n");
            break;
        }
        pool->QueueCapacity = queuesize;
        pool->Queuefront = 0;
        pool->Queueend = 0;

        if (pthread_mutex_init(&pool->MutexThreadPool, NULL) != 0 ||
            pthread_mutex_init(&pool->MutexBusyThread, NULL) != 0 ||
            pthread_cond_init(&pool->Full, NULL) != 0 ||
            pthread_cond_init(&pool->Empty, NULL) != 0) {
            printf("mutex or cond init fail.... /n");
            break;
        }
        // 管理者线程
        pthread_create(&pool->MonitorThread, NULL, monitor, pool);
        // 工作线程
        for (int i = 0; i < min; i++) {
            pthread_create(&pool->ThreadIDs[i], NULL, worker, pool);
        }
        pool->ShutDown = false;
        return pool;
    } while (0);
    // 内存分配失败，释放已经分配的内存
    if (pool && pool->ThreadIDs)
        free(pool->ThreadIDs);
    if (pool && pool->TaskQueue)
        free(pool->TaskQueue);
    free(pool);
    return NULL;
}

// 工作线程的函数
void* worker(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    while (1) {
        pthread_mutex_lock(&pool->MutexThreadPool);
        // 有虚假唤醒的风险，可以通过使用 while 循环重新检查条件是否满足来降低。
        // 虚假唤醒是说有好几个线程阻塞在这儿，pthread_cond_wait可能唤醒了不该唤醒的线程
        while (pool->QueueCapacity == 0 && pool->ShutDown == false) {
            // 阻塞工作线程(不空就把阻塞线程唤醒)
            pthread_cond_wait(&pool->Empty, &pool->MutexThreadPool);
            // 是否销毁线程
            if (pool->ExitThreadNum > 0) {
                pool->ExitThreadNum--;
                if (pool->LiveThreadNum > pool->MinThreadNum) {
                    pool->LiveThreadNum--;
                    pthread_mutex_unlock(&pool->MutexThreadPool);
                    ThreadExit(pool);
                }
            }
        }
        // 如果线程池被关闭
        if (pool->ShutDown == true) {
            // 避免死锁
            pthread_mutex_unlock(&pool->MutexThreadPool);
            ThreadExit(pool);
        }
        // 正常工作
        // 从队列中取出一个任务
        Task task;
        task.function = pool->TaskQueue[pool->Queuefront].function;
        task.arg = pool->TaskQueue[pool->Queuefront].arg;
        pool->Queuefront = (pool->Queuefront + 1) % pool->QueueCapacity;
        pool->QueueSize--;
        // 等待在 Full
        // 条件变量上的线程发送信号，通知它们任务队列已经不满了，可以继续添加任务。（只会唤醒其中的一个线程）
        // 如果想全部唤醒使用pthread_cond_broadcast 函数
        pthread_cond_signal(&pool->Full);
        // 解锁
        pthread_mutex_unlock(&pool->MutexThreadPool);

        printf("thread %ld start working...\n", pthread_self());
        pthread_mutex_lock(&pool->MutexBusyThread);
        pool->BusyThreadNum++;
        pthread_mutex_unlock(&pool->MutexBusyThread);
        // 执行任务
        task.function(task.arg);
        free(task.arg);
        // 防止野指针
        task.arg = NULL;

        printf("thread %ld end working...\n", pthread_self());
        pthread_mutex_lock(&pool->MutexBusyThread);
        pool->BusyThreadNum--;
        pthread_mutex_unlock(&pool->MutexBusyThread);
    }
}

// 管理者线程任务函数
/*实现线程数快不够或者过多时进行创建或者销毁，这样效率更高*/
void* monitor(void* arg) {
    ThreadPool* pool = (ThreadPool*)arg;
    while (pool->ShutDown == true) {
        // 每隔五秒检测一次
        sleep(5);

        // 取出线程池任务数量和当前线程数量
        pthread_mutex_lock(&pool->MutexThreadPool);
        int pool_task = pool->QueueSize;
        int liveNum = pool->LiveThreadNum;
        pthread_mutex_unlock(&pool->MutexThreadPool);
        // 这儿需要给线程池加锁，因为其他的work线程可能更改这些数据，但是前面的ShuntDown不需要加锁，因为正常工作时会被重复赋值false，对判断无影响。
        pthread_mutex_lock(&pool->MutexBusyThread);
        int busythread_num = pool->BusyThreadNum;
        pthread_mutex_unlock(&pool->MutexBusyThread);

        // 比较两数判断是否需要销毁/添加线程
         // 添加线程
        // 任务的个数>存活的线程个数 && 存活的线程数<最大线程数
        if (pool_task > liveNum && liveNum < pool->MaxThreadNum)
        {
            pthread_mutex_lock(&pool->MutexThreadPool);
            int counter = 0;
            for (int i = 0; i < pool->MaxThreadNum && counter < NUMBER
                && pool->LiveThreadNum < pool->MaxThreadNum; ++i)
            {
                if (pool->ThreadIDs[i] == 0)
                {cd
                    pthread_create(&pool->ThreadIDs[i], NULL, worker, pool);
                    counter++;
                    pool->LiveThreadNum++;
                }
            }
            pthread_mutex_unlock(&pool->MutexThreadPool);
        }
        // 销毁线程
        // 忙的线程*2 < 存活的线程数 && 存活的线程>最小线程数
        if (busythread_num * 2 < liveNum && liveNum > pool->MinThreadNum)
        {
            pthread_mutex_lock(&pool->MutexThreadPool);
            pool->ExitThreadNum = NUMBER;
            pthread_mutex_unlock(&pool->MutexThreadPool);
            // 让工作的线程自杀
            for (int i = 0; i < NUMBER; ++i)
            {
                pthread_cond_signal(&pool->Empty);
            }
        }
    }
    return NULL;
}

void ThreadExit(ThreadPool* pool) {
    // 获取当前线程ID
    pthread_t tid = pthread_self();
    for (int i = 0; i < pool->MaxThreadNum; ++i) {
        if (pool->ThreadIDs[i] == tid) {
            pool->ThreadIDs[i] = 0;
            printf("threadExit() called, %ld exiting...\n", tid);
            break;
        }
    }
    pthread_exit(NULL);
}
