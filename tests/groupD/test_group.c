
int sum(int a, int b) { return 99; } // declaration

int div(int a, int b)
{
	return sum(a,b);
}


int subs(int a, int b) {
	return a-b;
}

int mult(int a, int b) {
	return a*b;
}

int op(int a, int b, int c, int d) {
	return mult(a,b) + subs(c,d);
}

int gint;

int get_gint() {
	return gint;
}
