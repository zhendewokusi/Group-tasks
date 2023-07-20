#ifndef _THREADPOOL_H
#define _THREADPOOL_H

// 创建线程池并且初始化

struct ThreadPool;
typedef struct ThreadPool ThreadPool;
ThreadPool* ThreadPoolCreat(int max, int min, int queuesize);

// 销毁线程池

// 给线程池添加任务

// 获取当前线程池工作线程个数

// 获取线程池中存活线程个数

// 工作函数
void* worker(void* arg);
// 管理者线程任务函数
void* monitor(void* arg);
// 单个线程退出
void ThreadExit(ThreadPool* pool);

#endif  //   _THREADPOOL_H
