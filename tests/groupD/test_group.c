
int sum(int a, int b) { return 99; } // declaration

int div(int a, int b)
{
	return sum(a,b);
}


int subs(int a, int b) {
	return a-b;
}

int gint;

int get_gint() {
	return gint;
}

float floating() {
	return 3.5;
}

#include <stdio.h>

char c;

void hello() {
	printf("hello world\n");
}

void msg0() {
	printf("I am in function %s\n", __func__);
}

void msg1() {
	printf("I am in function %s\n", __func__);
	msg0();
}

void msg2() {
	printf("I am in function %s\n", __func__);
	msg1();
}


int mult(int a, int b) {
	return a*b;
}

char * get_ptr() {
	return &c;
}

void print_ptr() {
	printf("%p, %d\n", get_ptr(), (int)get_ptr());
}

int op(int a, int b, int c, int d) {
	return mult(a,b) + subs(c,d);
}
