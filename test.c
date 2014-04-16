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

unsigned t2(unsigned a, unsigned b) {
	printf("%s: a: %u\n",__func__,a);
	printf("%s: b: %u\n",__func__,b);
	return a + b;
}

short t3(short a,short b) {
	printf("%s: a: %d\n",__func__,a);
	printf("%s: b: %d\n",__func__,b);
	return a + b;
}

unsigned short t4(unsigned short a, unsigned short b) {
	printf("%s: a: %u\n",__func__,a);
	printf("%s: b: %u\n",__func__,b);
	return a + b;
}

long t5(long a, long b) {
	printf("%s: a: %d\n",__func__,a);
	printf("%s: b: %d\n",__func__,b);
	return a + b;
}

unsigned long t6(unsigned long a, unsigned long b) {
	printf("%s: a: %u\n",__func__,a);
	printf("%s: b: %u\n",__func__,b);
	return a + b;
}

long long t7(long long a, long long b) {
	printf("%s: a: %d\n",__func__,a);
	printf("%s: b: %d\n",__func__,b);
	return a + b;
}

unsigned long long t8(unsigned long long a, unsigned long long b) {
	printf("%s: a: %u\n",__func__,a);
	printf("%s: b: %u\n",__func__,b);
	return a + b;
}

