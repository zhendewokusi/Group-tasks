#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define THREAD_NUM 10

typedef struct task {
    int num;
    struct task* next;
} task_t;

/*线程池*/
typedef struct thread_pool {
    int thread_num;
    int queue_size;
    pthread_t* threads;
    task_t* queue_head;
    task_t* queue_tail;
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_not_empty;
    pthread_cond_t queue_not_full;
} thread_pool_t;

/*定义了每个线程如何处理任务*/
// work函数
/*    1.从队列中获取新任务并一个接一个地处理，直到没有剩余的任务为止。
      2.在成功处理每个任务后，会将其从内存中释放。*/

// 最好检查每个任务的返回值，只有在任务处理成功时才释放内存。

void* worker(void* arg) {
    int oldstate;
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, &oldstate);
    // 初始化线程池
    thread_pool_t* tp = (thread_pool_t*)arg;
    task_t* task;
    while (1) {
        pthread_mutex_lock(&tp->queue_lock);
        // 如果队列是空的就进行等待
        while (tp->queue_head == NULL) {
            pthread_cond_wait(&tp->queue_not_empty, &tp->queue_lock);
        }
        // 非空就把第一个任务传递出来
        task = tp->queue_head;
        // ！如果头尾位置一致就初始化！
        if (tp->queue_head == tp->queue_tail) {
            tp->queue_head = tp->queue_tail = NULL;
        } else {
            tp->queue_head = task->next;
        }
        // 向所有等待在queue_not_full条件变量上的线程发送信号，表示队列不再是满的。
        if (tp->queue_size == tp->thread_num - 1) {
            pthread_cond_broadcast(&tp->queue_not_full);
        }
        // 队列数目减1
        tp->queue_size--;
        pthread_mutex_unlock(&tp->queue_lock);
        // 输出线程号和任务数
        printf("Thread %lu: process task %d\n", pthread_self(), task->num);
        // 释放为任务分配的内存
        free(task);
    }
}

/*初始化线程池*/
// thread_pool_init函数
/*通过分配线程内存、定义互斥量和条件信号变量的值以及使用 pthread_create()
 * 创建具有指定功能的工作线程来实现。如果出现任何错误，则返回 -1。*/
int thread_pool_init(thread_pool_t* tp, int thread_num) {
    int i;
    // 线程池中的线程数目
    tp->thread_num = thread_num;
    // 队列数量初始化
    tp->queue_size = 0;
    // 队列头、尾初始化
    tp->queue_head = tp->queue_tail = NULL;

    pthread_mutex_init(&tp->queue_lock, NULL);
    pthread_cond_init(&tp->queue_not_empty, NULL);
    pthread_cond_init(&tp->queue_not_full, NULL);
    // 存储线程池中所有线程的 ID 号
    tp->threads = malloc(sizeof(pthread_t) * thread_num);
    // 失败就报错返回-1
    if (tp->threads == NULL) {
        perror("malloc threads");
        return -1;
    }
    // 创建了 thread_num 个工作线程
    for (i = 0; i < thread_num; i++) {
        // 线程都调用 worker() 函数
        if (pthread_create(&tp->threads[i], NULL, worker, tp) != 0) {
            perror("pthread_create");
            return -1;
        }


    }
    return 0;
}

// 更好的方法是使用标志变量来指示线程应正常退出。

// 取消活动线程，清楚队列任务，销毁互斥量和条件信号变量
//  pthread_cleanup_pop(execute);
//  pthread_cleanup_push(routine, arg);
void thread_pool_destroy(thread_pool_t* tp) {
    int i;
    task_t* task;
    // 取消线程，以确保它们安全退出
    for (i = 0; i < tp->thread_num; i++) {
        // pthread_cancel(tp->threads[i]);
        
    }
    // 等待线程退出
    for (i = 0; i < tp->thread_num; i++) {
        pthread_join(tp->threads[i], NULL);
    }

    // 遍历任务队列中的任务并释放其内存
    while (tp->queue_head != NULL) {
        task = tp->queue_head;
        tp->queue_head = tp->queue_head->next;
        free(task);
    }
    // 销毁互斥锁和条件变量，释放线程数组的内存
    pthread_mutex_destroy(&tp->queue_lock);
    pthread_cond_destroy(&tp->queue_not_empty);
    pthread_cond_destroy(&tp->queue_not_full);
    free(tp->threads);
}

// thread_pool_task
// 函数在添加新任务之前不会检查线程池是否已初始化。如果线程池未正确设置，这可能会导致分段错误或其他错误。

// 向任务队列添加新任务
// thread_pool_task函数
/*锁定互斥量，队列满了就等待，将任务添加到队列尾部，向工作线程发信号表示队列非空*/
int thread_pool_task(thread_pool_t* tp, int task_num) {
    // 为任务分配内存
    task_t* task = malloc(sizeof(task_t));
    // 分配失败报错返回
    if (task == NULL) {
        perror("malloc task");
        return -1;
    }
    // task初始化
    task->num = task_num;
    task->next = NULL;
    // 函数锁定了线程池的队列互斥量
    // queue_lock，以确保在修改队列时不会出现竞争条件。
    pthread_mutex_lock(&tp->queue_lock);
    // 检查队列是否已满，如果满了就等待
    while (tp->queue_size == tp->thread_num) {
        pthread_cond_wait(&tp->queue_not_full, &tp->queue_lock);
    }
    // 如果没有任务，就说明队列是空的，就将该任务设为队列的第一个节点
    if (tp->queue_head == NULL) {
        tp->queue_head = tp->queue_tail = task;
    }
    // 不是第一个节点，就添加到队列后
    else {
        tp->queue_tail->next = task;
        tp->queue_tail = task;
    }
    // 队列长度加1
    tp->queue_size++;
    // 和生产者消费者一样，不能把顺序搞反，不然容易形成死锁
    // 通知工作线程有新任务可用
    pthread_cond_signal(&tp->queue_not_empty);
    // 解除互斥锁
    pthread_mutex_unlock(&tp->queue_lock);
    return 0;
}

int main() {
    int i;
    thread_pool_t tp;
    sleep(2);
    if (thread_pool_init(&tp, THREAD_NUM) != 0) {
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < 20; i++) {
    // while(1){
        thread_pool_task(&tp, i);
        sleep(1);
    }

    thread_pool_destroy(&tp);
    printf("done\n");
    return 0;
}
