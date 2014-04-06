void do_math(int *x) {
  *x += 5;
}

int sum(int a, int b) {
	return a+b;
}

int subs(int a, int b) {
	return a-b;
}

int mult(int a, int b) {
	return a*b;
}

int do_inc(int *x);

int main(void) {
  int result = -1, val = 4;
  do_math(&val);
  do_inc(&val);
  return result;
}

int do_inc(int *x) {
  *x += 1;
  return 1;
}


