#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define THRNUM 20

// Single-producer , single-consumer Queue
// 单生产单消费者问题

struct SPSCQueue {
    void** buffer;              // 队列中的缓存数组
    int capacity;               // 缓存数组的容量
    int size;                   // 当前缓存数组中的元素数量
    int head;                   // 队列头指针
    int tail;                   // 队列尾指针
    pthread_mutex_t mutex;      // 互斥锁，用于线程同步
    pthread_cond_t cond_full;   // 条件变量，用于等待队列不满
    pthread_cond_t cond_empty;  // 条件变量，用于等待队列不空
} typedef SPSCQueue;

// 声明区：
SPSCQueue* SPSCQueueInit(int capacity);  // 初始化 参数：队列容量
void SPSCQueuePush(SPSCQueue* queue,
                   void* s);  // 添加队列信息，参数：队列，任务 采用头插法
void* SPSCQueuePop(
    SPSCQueue* queue);  // 取出队列内的内容 参数：队列 返回值：取出的内容
void SPSCQueueDestory(SPSCQueue* queue);  // 销毁
void* producer(void* arg);                // 生产者线程
void* consumer(void* arg);                // 消费者线程



void* producer(void* arg) {
    SPSCQueue* queue = (SPSCQueue*)arg;

    for (int i = 0; i < THRNUM; ++i) {
        // 生成一个随机数作为元素添加到队列中
        int* num = (int*)malloc(sizeof(int));
        *num = rand() % 100;
        printf("Producer: pushing %d\n", *num);
        SPSCQueuePush(queue, num);
    }

    return NULL;
}

// 消费者线程
void* consumer(void* arg) {
    SPSCQueue* queue = (SPSCQueue*)arg;

    for (int i = 0; i < THRNUM; ++i) {
        int* num = (int*)SPSCQueuePop(queue);
        printf("Consumer: popping %d\n", *num);
        free(num);
    }

    return NULL;
}

SPSCQueue* SPSCQueueInit(int capacity) {
    // 对一个任务节点分配内存
    SPSCQueue* queue = (SPSCQueue*)malloc(sizeof(SPSCQueue));
    queue->buffer = (void**)malloc(sizeof(void*) * capacity);
    // 队列属性的初始化
    queue->capacity = capacity;
    queue->size = 0;
    queue->head = 0;
    queue->tail = 0;
    // 互斥量，条件变量的初始化
    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->cond_full, NULL);
    pthread_cond_init(&queue->cond_empty, NULL);

    return queue;
}

void SPSCQueuePush(SPSCQueue* queue, void* s) {
    // 上锁
    pthread_mutex_lock(&queue->mutex);

    // 临界区
    // 若此时队列满 一直等待 直到有空
    while (queue->size == queue->capacity) {
        pthread_cond_wait(&queue->cond_full, &queue->mutex);
    }
    queue->buffer[queue->tail] = s;
    queue->tail = (queue->tail + 1) % queue->capacity;
    queue->size++;
    pthread_cond_signal(&queue->cond_empty);

    // 解锁
    pthread_mutex_unlock(&queue->mutex);
}

void* SPSCQueuePop(SPSCQueue* queue) {
    // 上锁
    pthread_mutex_lock(&queue->mutex);

    // 临界区
    // 若此时队列为空 一直等待 直到不空
    while (queue->size == 0) {
        pthread_cond_wait(&queue->cond_empty, &queue->mutex);
    }
    void* s = queue->buffer[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->size--;
    pthread_cond_signal(&queue->cond_full);

    // 解锁
    pthread_mutex_unlock(&queue->mutex);

    return s;
}

void SPSCQueueDestory(SPSCQueue* queue) {
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond_empty);
    pthread_cond_destroy(&queue->cond_full);

    free(queue->buffer);
    free(queue);
}

// Multi-producer , Multi-consumer Queue
struct MPMCQueue {
    /* Define Your Data Here */
} typedef MPMCQueue;

MPMCQueue* MPMCQueueInit(
    int capacity);  // 使用给定容量初始化 MPMCQueue 的新实例。
void MPMCQueuePush(MPMCQueue* queue, void* s);  // 将项目“s”添加到队列的末尾。
void* MPMCQueuePop(MPMCQueue* queue);  // 从队列前面删除并返回第一项。
void MPMCQueueDestory(MPMCQueue*);  // 销毁 MPMCQueue。
