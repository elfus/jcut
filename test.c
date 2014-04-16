#include <stdio.h>

void do_math(int *x) {
  printf("%s: X before: %d\n",__func__,*x);
  *x += 5;
  printf("%s: X after: %d\n",__func__,*x);
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
	printf("%s: X before: %d\n",__func__,*x);
  *x += 1;
  printf("%s: X after: %d\n",__func__,*x);
  return 1;
}


int mult(int a, int b) {
	return a*b;
}

int t1(int a, int *b, int c, int * d) {
	*b += 1;
	*d += 1;
	return a + *b + c + *d;
}
