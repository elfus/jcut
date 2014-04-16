#include <stdio.h>

void do_math(int *x) {
  printf("X before: %d\n",*x);
  *x += 5;
  printf("X after: %d\n",*x);
}

int sum(int a, int b) {
	return a+b;
}

int subs(int a, int b) {
	return a-b;
}

int mult(int a, int b);

void hello()
{
	printf("Hola mundo!\n");
}

int do_inc(int *x);

int main(void) {
  int result = -1, val = 4;
  int * x = &val;
  do_math(&val);
  do_inc(&val);
  printf("mult() = %d\n", mult(2,val));
  return result;
}

int do_inc(int *x) {
  *x += 1;
  return 1;
}


int mult(int a, int b) {
	return a*b;
}
