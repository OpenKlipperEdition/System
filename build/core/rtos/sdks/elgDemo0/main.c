#include <stdio.h>
extern int foo(int a);
int main(int argc, char **argv)
{
  printf("Hello world\n");
  foo(10);
  return 0;
}
