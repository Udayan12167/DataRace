#include <omp.h>

int main(){
	omp_lock_t writelock;
	int i;
	omp_init_lock(&writelock);

	#pragma omp parallel for
	for ( i = 0; i < 1; i++ )
	{
	    // some stuff
	   omp_set_lock(&writelock);
	    // one thread at a time stuff
	    omp_unset_lock(&writelock);
	    // some stuff
	}

	omp_destroy_lock(&writelock);
}