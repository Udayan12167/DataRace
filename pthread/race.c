#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>

pthread_t tid[10];
int counter_t;

void* doSomeThing(void *arg)
{
    unsigned long i = 0;
    counter_t += 1;
    printf("\n Job %d started\n", counter_t);

    for(i=0; i<50;i++);

    printf("\n Job %d finished\n", counter_t);


    return NULL;
}

int main(void)
{
    counter_t = 0;
    int i = 0;
    int err;


    while(i < 2)
    {
        err = pthread_create(&(tid[i]), NULL, &doSomeThing, NULL);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
        i++;
    }
    
    i = 0;
    while(i < 2){
	pthread_join(tid[i], NULL);
        i++;
    }	

    return 0;
}