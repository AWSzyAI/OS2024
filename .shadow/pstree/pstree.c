#include "pstree.h"

int main(int argc, char *argv[]) {
  readargs(argc, argv);
  cmd(argc, argv);
  assert(!argv[argc]);//确保命令行参数列表以空指针结尾，如果不是，则会触发断言错误。
  return 0;
}

