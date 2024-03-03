#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include "mygetopt.h"

void exe_n(int argc, char *argv[],int PID) {
  // 你原来在 -n 选项下的代码
}

void exe_p(int argc, char *argv[],int PID,int cntopt) {
  printf("argv[%d] = %s\n %d", cntopt, argv[cntopt],PID);
}

void exe_V(int argc, char *argv[],int PID) {
  printf("argv[%d] = %s %d\n", cntopt, argv[cntopt],PID);
}