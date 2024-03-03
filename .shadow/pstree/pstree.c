#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
    //execute according to the input
  }
  int cntopt=0;
  int opt;
  while((opt=getopt(argc,argv,"npV")!=-1){
    switch (opt)
    {
    case 'n':
      printf("argv[%d] = -n\n", cntopt);
      break;
    case 'p':
      printf("argv[%d] = -p\n", cntopt);
      break;
    case 'V':
      printf("argv[%d] = -V\n", cntopt);
      break;
    default:
      break;
    }
  }
  
  assert(!argv[argc]);//确保命令行参数列表以空指针结尾，如果不是，则会触发断言错误。
  return 0;
}
