#include <stdio.h>

short sum(int a, int b) {
	return a+b;
}

int subs(int a, int b) {
	return a-b;
}

float mult(float a, float b) {
	return a*b;
}

double div(double a, double b) {
	return a/b;
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

void msg() {
	char * a = 0;
	*a = 'a';
	printf("Hola mundo\n");
}
