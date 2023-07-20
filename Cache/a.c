#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

//常用的线程同步方式有四种：互斥锁、读写锁、条件变量、信号量。
// sem_wait set_post 条件变量
// pthread_mutex_lock pthread_mutex_unlock 互斥锁
//一般情况下，每一个共享资源对应一个把互斥锁，锁的个数和线程的个数无关。

/*条件变量应该在互斥锁前，因为如果先加锁，然后sem_wait就会堵塞*/



#define BUFFER_SIZE 10    // 缓冲区大小
int buffer[BUFFER_SIZE];  // 缓冲区数组
int in = 0;               // 缓冲区写入位置
int out = 0;              // 缓冲区读取位置

sem_t empty;            // 表示空槽的信号量
sem_t full;             // 表示已占用槽的信号量
pthread_mutex_t mutex;  // 用于控制缓冲区的访问
void produce() {
    int item = rand() % 1000;  // 生产一个随机整数
    sem_wait(&empty);          // 等待空槽
    pthread_mutex_lock(&mutex);  // 获取互斥锁，保证缓冲区的访问是互斥的
    buffer[in] = item;            // 将生产的数据写入缓冲区
    in = (in + 1) % BUFFER_SIZE;  // 更新缓冲区写入位置
    printf("Producing item : %d\n", item);
    pthread_mutex_unlock(&mutex);  // 释放互斥锁
    sem_post(&full);  // 发送已占用槽的信号量通知消费者
}

void consume() {
    int item;
    sem_wait(&full);             // 等待已占用槽
    pthread_mutex_lock(&mutex);  // 获取互斥锁，保证缓冲区的访问是互斥的
    item = buffer[out];             // 从缓冲区读取数据
    out = (out + 1) % BUFFER_SIZE;  // 更新缓冲区读取位置
    printf("Consuming item : %d\n", item);
    pthread_mutex_unlock(&mutex);  // 释放互斥锁
    sem_post(&empty);              // 发送空槽的信号量通知生产者
}

void* producer(void* arg) {
    while (1) {
        produce();
        sleep(rand() % 2);
    }
}

void* consumer(void* arg) {
    while (1) {
        consume();
        sleep(rand() % 2);
    }
}

int main(int argc, char* argv[]) {
    srand(time(NULL));                 // 初始化随机数生成器
    sem_init(&empty, 0, BUFFER_SIZE);  // 初始化空槽信号量为BUFFER_SIZE
    sem_init(&full, 0, 0);             // 初始化已占用槽信号量为0
    pthread_mutex_init(&mutex, NULL);  // 初始化互斥锁
    pthread_t pid, cid;
    pthread_create(&pid, NULL, producer, NULL);  // 创建生产者线程
    pthread_create(&cid, NULL, consumer, NULL);  // 创建消费者线程
    pthread_join(pid, NULL);                     // 等待生产者线程结束
    pthread_join(cid, NULL);                     // 等待消费者线程结束
    pthread_mutex_destroy(&mutex);               // 销毁互斥锁
    sem_destroy(&empty);                         // 销毁信号量
    sem_destroy(&full);
    return 0;
}