#include <stdio.h>

int GLOBAL_VARIABLE = 10;

void do_cmath(char *x) {
  printf("%s: X before: %d\n",__func__,*x);
  *x += 5;
  printf("%s: X after: %d\n",__func__,*x);
}

void do_math(int *x) {
  printf("%s: X before: %d\n",__func__,*x);
  *x += 5;
  printf("%s: X after: %d\n",__func__,*x);
}

void do_ucmath(unsigned char *x) {
  printf("%s: X before: %u\n",__func__,*x);
  *x += 5;
  printf("%s: X after: %u\n",__func__,*x);
}

void do_short_math(short int *x) {
  printf("%s: X before: %d\n",__func__,*x);
  *x += 5;
  printf("%s: X after: %d\n",__func__,*x);
}

void do_long_math(long int *x) {
  printf("%s: X before: %ld\n",__func__,*x);
  *x += 5;
  printf("%s: X after: %ld\n",__func__,*x);
}

void do_umath(unsigned int *x) {
  printf("%s: X before: %d\n",__func__,*x);
  *x += 5;
  printf("%s: X after: %d\n",__func__,*x);
}

void do_ushort_math(unsigned short int *x) {
  printf("%s: X before: %d\n",__func__,*x);
  *x += 5;
  printf("%s: X after: %d\n",__func__,*x);
}

void do_ulong_math(unsigned long int *x) {
  printf("%s: X before: %lu\n",__func__,*x);
  *x += 5;
  printf("%s: X after: %lu\n",__func__,*x);
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

char t0s(char a, char b) {
	printf("%s: a: %d\n",__func__,a);
	printf("%s: b: %d\n",__func__,b);
	return a + b;	
}

unsigned char t0u(unsigned char a, unsigned char b) {
	printf("%s: a: %u\n",__func__,a);
	printf("%s: b: %u\n",__func__,b);
	return a + b;	
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
	printf("%s: a: %ld\n",__func__,a);
	printf("%s: b: %ld\n",__func__,b);
	return a + b;
}

unsigned long t6(unsigned long a, unsigned long b) {
	printf("%s: a: %lu\n",__func__,a);
	printf("%s: b: %lu\n",__func__,b);
	return a + b;
}

long long t7(long long a, long long b) {
	printf("%s: a: %lld\n",__func__,a);
	printf("%s: b: %lld\n",__func__,b);
	return a + b;
}

unsigned long long t8(unsigned long long a, unsigned long long b) {
	printf("%s: a: %llu\n",__func__,a);
	printf("%s: b: %llu\n",__func__,b);
	return a + b;
}

/// Whether you declare it as b[], b[N] or *b, LLVM will implement it as 
/// a pointer of the given type: i8* %b
void zeromem(unsigned char b[], unsigned size) {
	unsigned char *i = b;
	unsigned char *end = b + size;

	while( i < end)
		*i++ = 0x0;
}

void print_buffer(void *buf, unsigned size) {
	short step = 1;
	unsigned char *i = (unsigned char*) buf;
	unsigned char *end = (unsigned char*) buf + size;

	while(i < end) {
		printf("%x ", *i);
		if(step%10 == 0)
			printf("\n");
		++step;
		++i;
	}
	printf("\n");
}

void reverse_buffer(void *buf, unsigned size) {
	unsigned char tmp = 0x0;
	unsigned char *i = (unsigned char*) buf;
	unsigned char *j = (unsigned char*) buf + size-1;

	zeromem(buf,size);
	printf("ZEROMEM\n");
	print_buffer(buf,size);

	int k = 0;
	for(k=0; k<size; k++) {
		i[k] = k;
	}
	printf("BEFORE\n");
	print_buffer(buf,size);
	while(i<j) {
		tmp = *j;
		*j-- = *i;
		*i++ = tmp;
	}
	printf("AFTER\n");
	print_buffer(buf,size);
}

void print_state() {
	printf("GLOBAL_VARIABLE = 0x%x\n",GLOBAL_VARIABLE);
}

void pre_test() {
	printf("[PRE TEST]\n");
}

void post_test() {
	printf("[POST TEST]\n");
}

void global_pre_test() {
	printf("[GLOBAL PRE TEST]\n");
}

void global_post_test() {
	printf("[GLOBAL POST TEST]\n");
}
