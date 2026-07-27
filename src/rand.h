#define RAND_MAX 2147483647

static unsigned long rand_state = 1;

void srand(unsigned int seed) { rand_state = seed ? seed : 1; }

int rand(void) {
  rand_state = rand_state * 1664525UL + 1013904223UL;
  return (int)(rand_state & 0x7fffffffUL);
}
