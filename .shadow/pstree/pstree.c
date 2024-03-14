#include "pstree.h"


void cmd(int argc, char *argv[]) {
  int opt;
  int option_processed = 0; // 标志变量
  while((opt=getopt(argc,argv,"npV"))!=-1){
    switch (opt)
    {
    case 'n':
      // exe_n(argc, argv);
      option_processed = 1; // 设置标志变量
      break;
    case 'p':
      // exe_p(argc, argv);
      option_processed = 1; // 设置标志变量
      break;
    case 'V':
      exe_V(argc, argv);
      option_processed = 1; // 设置标志变量
      break;
    default:
      printf("<usage> pstree [-npV]\n");
      option_processed = 1; // 设置标志变量
      break;
    }
  }
  
  
  if(!option_processed &&optind == argc){
    //getopt()函数的全局变量optind是命令行参数的索引，即argv[]数组的索引
    printf("[Log] optind = %d No targetPID\n", optind); 
    cmd_root(argc,argv);
  }
}

int main(int argc, char *argv[]) {
  readargs(argc, argv);
  cmd(argc, argv);
  assert(!argv[argc]);//确保命令行参数列表以空指针结尾，如果不是，则会触发断言错误。
  return 0;
}

