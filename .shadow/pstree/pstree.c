#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
    //execute according to the input
  }

  //try getopt()
  printf("-----try getopt-----");
  int cntopt=0;
  int opt;
  while((opt=getopt(argc,argv,"npV"))!=-1){
    switch (opt)
    {
    case 'n':
      cntopt++;
      printf("argv[%d] = %s\n", cntopt, argv[cntopt]);
      break;
    case 'p':
      cntopt++;
      printf("argv[%d] = %s\n", cntopt, argv[cntopt]);
      break;
    case 'V':
      cntopt++;
      printf("argv[%d] = %s\n", cntopt, argv[cntopt]);
      break;
    default:
      printf("argv[%d] = %s\n", cntopt, argv[cntopt]);
      break;
    }
  }
  
  assert(!argv[argc]);//确保命令行参数列表以空指针结尾，如果不是，则会触发断言错误。
  return 0;
}
