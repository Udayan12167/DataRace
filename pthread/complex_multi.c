#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>

pthread_t tid[10];
int counter_t;
pthread_mutex_t lock;

void* doSomeThing(void *arg)
{
    pthread_mutex_lock(&lock);

    unsigned long i = 0;
    counter_t += 1;
    printf("\n Job %d started\n", counter_t);

    for(i=0; i<50;i++);

    printf("\n Job %d finished\n", counter_t);

    pthread_mutex_unlock(&lock);

    return NULL;
}

int main(void)
{
    counter_t = 0;
    int i = 0;
    int err;

    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

    while(i < 4)
    {
        err = pthread_create(&(tid[i]), NULL, &doSomeThing, NULL);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
        i++;
    }
    
    i = 0;
    while(i < 4){
	pthread_join(tid[i], NULL);
        i++;
    }	
    pthread_mutex_destroy(&lock);

    return 0;
}
