 /*有五个哲学家绕着圆桌坐，每个哲学家面前有一盘面，两人之间有一支筷子，这样每个哲学家左右各有一支筷子。哲学家有2个状态，思考或者拿起筷子吃饭。如果哲学家拿到一只筷子，不能吃饭，直到拿到2只才能吃饭，并且一次只能拿起身边的一支筷子。一旦拿起便不会放下筷子直到把饭吃完，此时才把这双筷子放回原处。如果，很不幸地，每个哲学家拿起他或她左边的筷子，那么就没有人可以吃到饭了。*/
 
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
//五个哲学家
#define N 5 
//THINGING状态是0 一个筷子都没有的情况
//HUNGRY状态是1   一个筷子饿肚子
//EATING状态是2   两个筷子吃饭饭
enum { THINKING, HUNGRY, EATING } state[N];
// 全局锁 
pthread_mutex_t mutex; 
//阻塞条件变量 
pthread_cond_t cond[N]; 

// 获取左边筷子的编号 
int LEFT(int i) {
  return (i + N - 1) % N;
}
//获取右边筷子的编号
int RIGHT(int i) {
  return (i + 1) % N;
}
// 函数检查当前的哲学家 （i） 是否饿了，而它的邻居哲学家是否还没有吃东西。如果是，那么它将当前哲学家的状态设置为进食，并向当前哲学家的条件变量发出信号以唤醒其他等待线程。
void test(int i) {
  if (state[i] == HUNGRY &&
      state[LEFT(i)] != EATING &&
      state[RIGHT(i)] != EATING) {
    state[i] = EATING;
    pthread_cond_signal(&cond[i]);
  }
}

void take_forks(int i) {
  pthread_mutex_lock(&mutex);
  state[i] = HUNGRY;
  test(i);

  while (state[i] != EATING) {
    pthread_cond_wait(&cond[i], &mutex);
  }

  pthread_mutex_unlock(&mutex);
}

void put_forks(int i) {
  pthread_mutex_lock(&mutex);
  state[i] = THINKING;
  test(LEFT(i));
  test(RIGHT(i));
  pthread_mutex_unlock(&mutex);
}

void* philosopher(void* arg) {
  int i = *(int*)arg;
  while(1) {
    int sleep_time = rand() % 5 + 1;
    printf("Philosopher %d is thinking\n", i);
    sleep(sleep_time);
    take_forks(i);
    //吃饭时间
    printf("Philosopher %d is eating\n", i);
    sleep_time = rand() % 5 + 1;
    sleep(sleep_time);
    //吃完放餐具
    put_forks(i);
  }
}

int main() {
  pthread_t tid[N];
  int i;
  int philosopher_id[N] = {0, 1, 2, 3, 4};

  pthread_mutex_init(&mutex, NULL);
  for (i = 0; i < N; i++) {
    pthread_cond_init(&cond[i], NULL);
  }

  for (i = 0; i < N; i++) {
    pthread_create(&tid[i], NULL, philosopher, &philosopher_id[i]);
  }

  for (i = 0; i < N; i++) {
    pthread_join(tid[i], NULL);
  }
  return 0;
}



