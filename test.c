#include <stdio.h>
#include <string.h>
#include <ctype.h>

void url_decode(char *str) {
  char *p = str;
  while (*p) {
    if (*p == '%') {
      if (p[1] && p[2]) {
        int value = 0;
        sscanf(p + 1, "%2x", &value);
        *p = (char)value;
        memmove(p + 1, p + 3, strlen(p) - 2);
      }
    }
    p++;
  }
}

int main(void) {
  char encoded_str[] = "filename%20with%25%20signs.txt";
  url_decode(encoded_str);
  printf("Decoded string: %s\n", encoded_str);
  return 0;
}