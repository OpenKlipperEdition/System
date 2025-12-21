#include <stdio.h>
#include <unistd.h>
int main(int argc, char **argv)
{
  char opt;
  while((opt = getopt(argc, argv, "m:p:j:o:s:i:")) != -1){
    switch (opt){
      case 'm':
        printf("opt: %c - %s\n", opt, optarg);
        break;
      case 'p': {
        printf("opt: %c - %s\n", opt, optarg);
        break;
      }
      case 'j': {
        printf("opt: %c - %s\n", opt, optarg);
        break;
      }
      case 'o': {
        printf("opt: %c - %s\n", opt, optarg);
        break;
      }
      case 's': {
        printf("opt: %c - %s\n", opt, optarg);
        break;
      }
      case 'i': {
        printf("opt: %c - %s\n", opt, optarg);
        break;
      }
    }
  }
  printf("1ending mkrtoslds2\n");
  return 0;
}
