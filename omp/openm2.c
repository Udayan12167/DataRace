#include<omp.h>

int main(){
	int x =0 ;
	#pragma omp parallel num_threads(2) shared(x)
	{
	#pragma omp atomic
	x++;
	}
	return 0;
}
