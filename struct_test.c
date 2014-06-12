#include <stdio.h>
#include <stdlib.h>

struct B {
	int a;
	int b;
};
typedef struct B b_t;

struct A{
	int a;
	short b;
	short c;
	b_t d;
	short e;
	b_t f;
} st1;
typedef struct A a_t;


int gint = 0;
a_t gstruct = {1,2,3,{10,20}, 0, {0, 0}};
a_t gstruct2 = {.c=3,.a=1, .d = {.b=20, .a=10 }, .b=2 };

void assignment() {
	gstruct.a = 255;
	gstruct.b = 2014;
}

void print_g() {
	printf("gint = %d\n",gint);
	printf("gstruct: a=%d, b=%d, c=%d, d.a=%d, d.b=%d, e=%d, f.a=%d, f.b=%d, \n",
		gstruct.a,gstruct.b, gstruct.c,
		gstruct.d.a, gstruct.d.b,
		gstruct.e, gstruct.f.a, gstruct.f.b);
	printf("gstruct2: a=%d, b=%d, c=%d, d.a=%d, d.b=%d\n",gstruct2.a,gstruct2.b, gstruct2.c,
		gstruct2.d.a, gstruct2.d.b);
}

//
//struct B {
//	struct A a_struct;
//}st_st;
//
//union U{
//	int a;
//	short b;
//	short c;
//} u1;
//
//struct Name {
//	char a;
//	short b;
//	int c;
//	struct B b_struct;
//	int *ptr;
//} st2;
//
//struct A * ptr_A;
//
//int comp() {
//	a_t a = {1,2,3};
//	a_t b = a;
//	a = b;
//	if (a.a == b.a)
//		return 0;
//	return 1;
//}
//
//void initU() {
//	u1.a = 1;
//	u1.b = 2;
//	u1.c = 3;
//}
//void init1_1() {
//	a_t a = { 10, 12, 14};
//}
//
//void init2_2() {
//	a_t a = { .a = 10, .b = 12, .c = 14};
//}
//
//void init3_3() {
//	a_t a = { .c = 14, .b = 12,  .a = 10 };
//}
//
//void init4_4() {
//	a_t a = { .c = 14, .a = 10 };
//}
//
//
//void init7() {
//	struct Name t1;
//	t1.a = 10;
//	t1.b = 20;
//	t1.c = 30;
//	t1.b_struct.a_struct.a = 0;
//}
//
//void alloc_struct() {
//	ptr_A = malloc(sizeof(struct A));
//}