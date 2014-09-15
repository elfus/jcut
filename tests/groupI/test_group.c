#include <stdio.h>

int gint;

void print(char *msg) {
	printf("%s\n",msg);
}

void print_double(char *msg1,char *msg2) {
	printf("%s%s\n",msg1,msg2);
}

void msg() {
	printf("%s: hello, gint = %d\n",__func__,gint);
}
int get_gint() {
	printf("%s: gint == %d\n",__func__, gint);
	return gint;
}

void global_setup() {
	printf("%s: hello global setup\n",__func__);
}

void global_teardown() {
	printf("%s: hello global teardown\n",__func__);
}

short sum(int a, int b) {
	return a+b;
}

int subs(int a, int b) {
	return a-b;
}

float mult(float a, float b) {
	return a*b;
}

char t0s(char a, char b) {
	printf("%s: a: %d\n",__func__,a);
	printf("%s: b: %d\n",__func__,b);
	printf("%d\n",a+b);
	int x = (int)a;
	return a + b;	
}

unsigned char t0u(unsigned char a, unsigned char b) {
	printf("%s: a: %u\n",__func__,a);
	printf("%s: b: %u\n",__func__,b);
	return a + b;	
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


void print_ptr_char(char *c)
{
	printf("%s: %x\n",__func__, *c);
}

void print_bfr(void *b, unsigned size)
{
	char *c;
	while(size--) {
		c = (char*) b++;
		printf("0x%x ",*c);
	}
	printf("\n");
}

void do_cmath(char *x) {
	printf("%s: X = %p\n", __func__, x);
	if (x) {
		printf("%s: X before: %d\n",__func__,*x);
		*x += 5;
		printf("%s: X after: %d\n",__func__,*x);
	} else
		printf("%s: Invalid pointer to do math!\n",__func__);
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

void do_float_math(float *x) {
  printf("%s: X before: %f\n",__func__,*x);
  *x += 5;
  printf("%s: X after: %f\n",__func__,*x);
}

void do_double_math(double *x) {
  printf("%s: X before: %f\n",__func__,*x);
  *x += 5;
  printf("%s: X after: %f\n",__func__,*x);
}

/// @return 0 when the buffer of size size is initialized to val, 1 otherwise
int is_initialized(void *b, unsigned size, char val)
{
	char *c;
	while(size--) {
		c = (char*) b++;
		if (*c != val)
			return 1;
	}
	return 0;
}

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
//testing with global pointers
int* gptr_int = 0;
int gint = 0;
int** gpp_int = 0;

void print_gptr_int() {
	printf("%p (0x%x)\n",gptr_int,(unsigned)gptr_int);
}

unsigned get_gptr_adrr() {
	return (unsigned)gptr_int;
}

int math_gptr_int() {
	if (gptr_int) {
		printf("%s: Valid pointer!\n",__func__);
		do_math(gptr_int);
		return 0;
	} else {
		printf("%s: Invalid pointer!\n",__func__);
	}
	printf("Returning 1\n");
	return 1;
}

unsigned get_gpp_addr() {
	return (unsigned)gpp_int;
}

int math_gpp_int() {
	printf("%s: gpp_int = %p\n",__func__,gpp_int);
	if (gpp_int) {
		printf("%s: Valid pointer to pointer!\n",__func__);
		printf("%s: (*gpp_int) == %p\n",__func__,*gpp_int);
		if (*gpp_int != 0) {
			do_math(*gpp_int);
		}
		return 0;
	} else {
		printf("%s: Invalid pointer to pointer!\n",__func__);
		return 1;
	}
}

#include <stdlib.h>

void pp_math(int **x)
{
	printf("%s: x = %p\n", __func__,x);
	printf("%s: *x = %p\n", __func__,*x);
	if (*x == 0) {
		printf("Allocation 1 pointer\n");
		*x = (int*) malloc(sizeof(int));
		printf("%s: *x = %p [malloc]\n", __func__,*x);
		if ( *x != 0) {
			**x = 0;
			do_math(*x);
			free(*x);
		}
	}
}

// Pointers to functions
typedef int (*ptr_func) ();

ptr_func pf;

unsigned get_ptr_func_addr() {
	return (unsigned)pf;
}

int print_str(char * s) {
    printf("s = %p\n",s);
    if(s) {
        printf("%s\n",s);
        return 0;
    }
    else {
        printf("Invalid pointer!\n");
        return -1;
    }
}
