#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <semaphore.h>
#include <pthread.h>

#include "dr_detector.h"

#define BUF_SIZE 5       // 缓冲区大小
#define MAX_SEM_VALUE 5  // 信号量最大值

int buffer[BUF_SIZE];    // 缓冲区
int in = 0;              // 插入位置
int out = 0;             // 取出位置
sem_t empty_sem;         // 空缓冲区计数信号量
sem_t full_sem;          // 满缓冲区计数信号量
pthread_mutex_t mutex;   // 缓冲区互斥锁

void *producer(void *arg)
{
    while (true) {
        int item = rand() % 100 + 1;
        //sem_wait(&empty_sem);
        pthread_mutex_lock(&mutex);
        if (in < BUF_SIZE) {
            buffer[in++] = item;
            //std::cout << "生产者：插入 " << item << " 到缓冲区 [" << out << ", " << in << ")" << std::endl;
        }
        pthread_mutex_unlock(&mutex);
        int sem_cnt = -1;
        sem_getvalue(&full_sem,&sem_cnt);
        //std::cout << "信号量:" << sem_cnt << std::endl;
        sem_post(&full_sem);
        usleep(1 % 1000);
    }
}

void *consumer(void *arg)
{
    while (true) {
        int item;
        sem_wait(&full_sem);
        pthread_mutex_lock(&mutex);
        if (out < in) {
            item = buffer[out++];
            //std::cout << "消费者：取出 " << item << " 从缓冲区 [" << out << ", " << in << ")" << std::endl;
        }
        pthread_mutex_unlock(&mutex);
        sem_post(&empty_sem);
        usleep(2 % 1000);
    }
}

// 检测器列表，越前面的检测器速度越快，优先级越高。每当有新的图片进来，要按照优先级来把图片分配给检测器。
// 当速度最快的检测器忙的时候，才尝试使用低速检测器。
// 最低速的检测器不做任何处理，只是记录图片，用于利用前面检测器的结果进行tracking
// 如果所有的检测器都满了，那就丢弃
DrDetector detectors[]={
    DrDetector(3,"detector0"),
    DrDetector(5,"detector1"),
};

int main(int argc, char **argv)
{
    srand(time(nullptr));

    // 初始化信号量和互斥锁
    sem_init(&empty_sem, 0, MAX_SEM_VALUE);  //初始化为空的信号量
    sem_init(&full_sem, 0, 0);    //初始化满的信号量
    pthread_mutex_init(&mutex, nullptr);

    // 启动多个生产者和消费者线程
    pthread_t p1, p2, c1, c2;
    pthread_create(&p1, nullptr, producer, nullptr);
    //pthread_create(&p2, nullptr, producer, nullptr);
    pthread_create(&c1, nullptr, consumer, nullptr);
    //pthread_create(&c2, nullptr, consumer, nullptr);

    // 等待线程结束
    pthread_join(p1, nullptr);
    //pthread_join(p2, nullptr);
    pthread_join(c1, nullptr);
    //pthread_join(c2, nullptr);

    // 销毁信号量和互斥锁
    sem_destroy(&empty_sem);
    sem_destroy(&full_sem);
    pthread_mutex_destroy(&mutex);

    return 0;
}