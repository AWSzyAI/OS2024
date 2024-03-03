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

int main(int argc, char *argv[]) {
  int targetPID = 1;//default PID is 1
  
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
    //execute according to the input
  }

  //try getopt()
  printf("-----try getopt-----\n");
  int cntopt=0;
  int opt;
  while((opt=getopt(argc,argv,"npV"))!=-1){
    switch (opt)
    {
    case 'n':
      cntopt++;
      targetPID = atoi(argv[cntopt+1]);//targetPID is int(the next argument)
      printf("argv[%d] = %s %d\n", cntopt, argv[cntopt],targetPID);

      int *PIDs = NULL;
      PIDs = traverseProcDirectory();
      printf("PIDs : \n");
      for(int i=1;i<=PIDs[0];i++){
        printf("%d ", PIDs[i]);
      }
      puts("");
      
      printf("PID = %d\n", targetPID);

      printf("-----try to open /proc/%d/stat-----\n", targetPID);
      char filename[100];
      sprintf(filename, "/proc/%d/stat", targetPID);
      FILE *fp = fopen(filename, "r");
      if (fp == NULL) {
        printf("No such process\n");//pstree实际上不输出
        fclose(fp);
        return 0;
      }

      char line[MAX_LINE_LENGTH];
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
      
      fclose(fp);
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
