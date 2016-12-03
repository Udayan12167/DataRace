#include <pthread.h>
#include <stdio.h>


  
int main()
{
	printf("Pthread testing unlock");
	pthread_mutex_t  rwlock;
	pthread_mutex_init(&rwlock, NULL);
	pthread_mutex_lock(&rwlock);
	pthread_mutex_unlock(&rwlock);

	return 0;
}
