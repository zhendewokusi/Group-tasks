#include "a_1.c"

int main() {
    SPSCQueue* queue = SPSCQueueInit(THRNUM);

    pthread_t producer_tid;
    pthread_t consumer_tid;
    int i =1;
    pthread_create(&producer_tid, NULL, producer, queue);
    pthread_create(&consumer_tid, NULL, consumer, queue);

    pthread_join(producer_tid, NULL);
    pthread_join(consumer_tid, NULL);

    SPSCQueueDestory(queue);

    exit(0);
}
