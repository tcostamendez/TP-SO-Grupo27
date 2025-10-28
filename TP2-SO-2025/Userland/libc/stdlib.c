#include <stdlib.h>

// C's stdlib pseudo random number generator
// https://wiki.osdev.org/Random_Number_Generator

/* --------------- Random --------------- */
static unsigned long int next =
    1; // NB: "unsigned long int" is assumed to be 32 bits wide

int rand(void) { // RAND_MAX assumed to be 32767

  next = next * 1103515245 + 12345;
  return (unsigned int)(next / 65536) % 32768;
}

void srand(unsigned int seed) { next = seed; }

/* --------------- String --------------- */
int atoi(const char *str) {
  int result = 0;
  int sign = 1;
  int i = 0;

  // Skip whitespace
  while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n') {
    i++;
  }

  // Handle sign
  if (str[i] == '-') {
    sign = -1;
    i++;
  } else if (str[i] == '+') {
    i++;
  }

  // Convert digits
  while (str[i] >= '0' && str[i] <= '9') {
    result = result * 10 + (str[i] - '0');
    i++;
  }

  return sign * result;
}
