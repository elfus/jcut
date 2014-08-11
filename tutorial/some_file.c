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
