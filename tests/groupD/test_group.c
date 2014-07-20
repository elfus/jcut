#include <stdio.h>

int sum(int a, int b) { return 99; } // declaration

int get_int(int a, int b)
{
	int x = sum(a,b);
	printf("%s: func address %p, the result was: %d\n",__func__,sum, x);
	return x;
}

int div(int a, int b)
{
	return sum(a,b);
}