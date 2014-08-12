#include <stdio.h>

void my_function() {
  return;
}

int sum(int a, int b) {
  return a+b;
}

int fact_1(int a) {
  if(1 == a) return a;
  return a * fact_1(a-1);
}

int fact_2(int a) {
  unsigned i = 1;
  while(a) i *= a--;
  return i;
}

int state;

int get_state() {
  return state;
}

void modify_state() {
  state += 5;
}


struct Pair {
	int a;
	int b;
};

struct NestedPair {
	int a;
	int b;
	struct Pair nested;
};

struct NestedPair global_pair;

void print_global_pair() {
	printf("global_pair.a = %d\n", global_pair.a);
	printf("global_pair.b = %d\n", global_pair.b);
	printf("global_pair.subpair.a = %d\n", global_pair.nested.a);
	printf("global_pair.subpair.b = %d\n", global_pair.nested.b);
}

void print_pair(struct NestedPair pair) {
	printf("pair.a = %d\n", pair.a);
	printf("pair.b = %d\n", pair.b);
	printf("pair.subpair.a = %d\n", pair.nested.a);
	printf("pair.subpair.b = %d\n", pair.nested.b);
}
/******** pointers ************/
int * g_ptr = NULL;

void print_g_ptr(){
	if(g_ptr)
		printf("address: %p, contents: %d\n",g_ptr,*g_ptr);
	else
		printf("ERROR g_ptr is NULL pointer!\n");
}


void print_buffer(unsigned char *buf, unsigned size) {
	short step = 1;
	unsigned char *i = buf;
	unsigned char *end = buf + size;

	while(i < end) {
		printf("%x ", *i);
		if(step%10 == 0)
			printf("\n");
		++step;
		++i;
	}
	printf("\n");
}

struct NestedPair * pair_ptr = NULL;

void print_pair_ptr() {
	if(pair_ptr) {
		printf("pair_ptr.a = %d\n", pair_ptr->a);
		printf("pair_ptr.b = %d\n", pair_ptr->b);
		printf("pair_ptr.subpair.a = %d\n", pair_ptr->nested.a);
		printf("pair_ptr.subpair.b = %d\n", pair_ptr->nested.b);
	} else
		printf("ERROR pair_ptr is NULL pointer!\n");
}

void print_ptr_pair(struct NestedPair* ptr) {
	if(ptr) {
		printf("ptr.a = %d\n", ptr->a);
		printf("ptr.b = %d\n", ptr->b);
		printf("ptr.subpair.a = %d\n", ptr->nested.a);
		printf("ptr.subpair.b = %d\n", ptr->nested.b);
	} else
		printf("ERROR ptr is NULL pointer!\n");
}