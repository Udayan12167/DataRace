#include <sys/resource.h>
#include <stdio.h>
int main (void)
{
  int i=0;
  struct rlimit limit;

  getrlimit (RLIMIT_STACK, &limit);
  printf ("Stack Begginning = %p \nStack Limit = %ld and %ld max\n", &i, limit.rlim_cur, limit.rlim_max);
}
