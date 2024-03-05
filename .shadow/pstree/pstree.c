#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>



#define MAX_LINE_LENGTH 1024

typedef struct {
  int pid;
  char *name;
  int ppid;
}Process;

int isNumeric(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

int* traverseProcDirectory() {
  
  int MaxPID = 100;
  
  int *PIDs = (int*)malloc(MaxPID * sizeof(int));
  PIDs[0] = 0;
  int cnt = 1;
  DIR* dir = opendir("/proc");
  if (dir == NULL) {
    perror("opendir");
    return NULL;
  }

  struct dirent* entry;
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR) {
      if (isNumeric(entry->d_name)) {
        PIDs[0]++;
        if(cnt>=MaxPID){
          MaxPID*=2;
          PIDs = (int*)realloc(PIDs, MaxPID * sizeof(int));
        }
        PIDs[cnt] = atoi(entry->d_name);

        //printf("Directory: %s\n", entry->d_name);
        // 进行相应的操作
      }
    }
  }
  closedir(dir);
  return PIDs;
}


void exe_n(int argc, char *argv[],int cntopt) {

  int targetPID;
  printf("argc = %d\n", argc);
  if(argc==2){
    targetPID = 1;
    printf("No targetPID, use default PID = 1\n");
  }else{
    targetPID = atoi(argv[2]);
  }
 
  printf("-----try to open /proc/%d/stat-----\n", targetPID);
  char filename[100];
  
  sprintf(filename, "/proc/%d/stat", targetPID);
  FILE *fp = fopen(filename, "r");
  if(!fp)goto release;

  char line[MAX_LINE_LENGTH+1];
  fgets(line, MAX_LINE_LENGTH, fp);
  printf("line = %s\n", line);

  Process process;

  sscanf(line, "%d", &process.pid);//get PID
  char *token = strtok(line, " "); //跳过进程ID
  token = strtok(NULL, " "); 
  sscanf(token, "%s", process.name);//get name
  token = strtok(NULL, " "); 
  token = strtok(NULL, " "); 
  sscanf(token, "%d", &process.ppid);//get PPID
  
  printf("进程ID = %d\n", process.pid);
  printf("进程名 = %s\n", process.name);
  printf("父进程PID = %d\n",process.ppid);
    
  printf("-----try to open /proc/*-----\n");
  DIR *dir;
  struct dirent *entry;
  int count = 0;
  dir = opendir("/proc/");
  if(dir == NULL){
    perror("opendir error");
    return;
  }

  printf("PIDs : ");
  while((entry = readdir(dir)) != NULL){
    if(isNumeric(entry->d_name)){
      count++;
      int pid = atoi(entry->d_name);
      printf("%d ", pid);
    }
  }
  puts("");
  printf("count = %d\n", count);

  if(dir)closedir(dir);




  sprintf(filename, "/proc/%d/status", targetPID);
  fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("No such process\n");//pstree实际上不输出
    fclose(fp);
    return;
  }

release:  
  if(fp)fclose(fp);


 }


void exe_V(int argc, char*argv[],int cntopt){
  printf("pstree-32/64 (OS2024 - Ziyan Shi) version 0.0.1\nCopyright (C) 2024-2024 NJU and Ziyan Shi\nPSmisc comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it under the terms of the GNU General Public License.\nFor more information about these matters, see the files named COPYING\n");
}

void exe_root(int argc, char *argv[],int cntopt){
  printf("root————\n");

  int targetPID;
  printf("argc = %d\n", argc);
  if(argc==2){
    targetPID = 1;
    printf("No targetPID, use default PID = 1\n");
  }else{
    targetPID = atoi(argv[2]);
  }
 

  char filename[100];
  printf("-----try to open /proc/*-----\n");
  
  DIR *dir;
  struct dirent *entry;
  int count = 0;
  dir = opendir("/proc/");
  if(dir == NULL){
    perror("opendir error");
    return;
  }

  printf("PIDs : ");
  while((entry = readdir(dir)) != NULL){
    if(isNumeric(entry->d_name)){
      count++;
      int pid = atoi(entry->d_name);
      printf("%d ", pid);
    }
  }
  puts("");
  printf("count = %d\n", count);

  if(dir)closedir(dir);




  sprintf(filename, "/proc/%d/status", targetPID);
  FILE *fp;
  fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("No such process\n");//pstree实际上不输出
    fclose(fp);
    return;
  }

  if(fp)fclose(fp);


}

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  
  int opt;
  int cntopt=0;
  
  while((opt=getopt(argc,argv,"npV"))!=-1){
    switch (opt)
    {
    case 'n':
      printf("----exe_n----\n");
      exe_n(argc, argv, cntopt);
      printf("----exe_n----\n");
      
      // int *PIDs = NULL;
      // PIDs = traverseProcDirectory();
      // printf("PIDs : \n");
      // for(int i=1;i<=PIDs[0];i++){
      //   printf("%d ", PIDs[i]);
      // }
      // puts("");

     
      break;
    case 'p':
      cntopt++;
      printf("argv[%d] = %s\n", cntopt, argv[cntopt]);
      break;
    case 'V':
      exe_V(argc, argv, cntopt);
      break;
    default:
      printf("<usage> pstree [-npV]\n");
      break;
    }
  }

  printf("optind = %d\n", optind);//getopt()函数的全局变量optind是命令行参数的索引，即argv[]数组的索引
  if(optind == argc){
    printf("No targetPID\n");
    exe_root(argc,argv,cntopt);
  }


  
  assert(!argv[argc]);//确保命令行参数列表以空指针结尾，如果不是，则会触发断言错误。
  return 0;
}
