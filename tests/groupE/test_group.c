#include <stdio.h>

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
#include <stdlib.h>
//testing with global pointers
int* gptr_int = 0;
int gint = 0;

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
