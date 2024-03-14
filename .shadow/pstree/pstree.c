#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>


#define MAX_LINE_LENGTH 1024


typedef struct{
  int pid;
  char *name;
  int ppid;
}Process;

typedef struct psNode{
  int pid;
  char *name;
  int ppid;
  int depth;
  struct psNode *Parent;
  struct psNode *FirstSon;
  struct psNode *NextSibling;
}psNode;


psNode *root = NULL;

psNode* getNode(int pid, psNode *root){
  if(!root)return NULL;
  if(root->pid==pid)return root;
  struct psNode* targetNode=NULL;
  targetNode = getNode(pid,root->FirstSon);
  if(!targetNode){
    targetNode = getNode(pid,root->NextSibling);
  }
  return targetNode;
}

void addNode(int pid, int ppid, char *name){
  if(!root){
    root = (psNode*)malloc(sizeof(psNode));
    root->pid = pid;
    root->ppid = ppid;
    root->name = name;
    root->depth = 0;
    root->Parent = NULL;
    root->FirstSon = NULL;
    root->NextSibling = NULL;
  }
  psNode *p = getNode(ppid, root);
  psNode *q = (psNode*)malloc(sizeof(psNode));//new psNode(pid);
  q->pid = pid;
  q->ppid = ppid;
  q->name = name;
  q->depth = p->depth+1;
  q->Parent = p;
  q->FirstSon = NULL;
  q->NextSibling = NULL;
  if(!p->FirstSon){
    p->FirstSon = q;
  }else{
    psNode *t = p->FirstSon;
    while(t->NextSibling){
      t = t->NextSibling;
    }
    t->NextSibling = q;
  }
}


void PrintTree(psNode *root, int depth){
  if(!root) return;
  
  for(int i = 0; i < depth; i++)
    printf("  ");
  
  printf("%s\n", root->name);
  
  psNode *child = root->FirstSon;
  while(child){
    PrintTree(child, depth + 1);
    child = child->NextSibling;
  }
}



int getPPID(int targetPID){
  char filename[100];
  sprintf(filename, "/proc/%d/stat", targetPID);
  FILE *fp = fopen(filename, "r");
  if(!fp){
    printf("No such process\n");
    return -1;
  }

  char line[MAX_LINE_LENGTH+1];
  fgets(line, MAX_LINE_LENGTH, fp);
  // printf("line = %s\n", line);

  Process process;

  sscanf(line, "%d", &process.pid);//get PID
  char *token = strtok(line, " "); //跳过进程ID
  token = strtok(NULL, " "); 

  // Allocate memory for process.name
  process.name = (char*)malloc(MAX_LINE_LENGTH * sizeof(char));
  sscanf(token, "%s", process.name);//get name

  token = strtok(NULL, " "); 
  token = strtok(NULL, " "); 
  sscanf(token, "%d", &process.ppid);//get PPID
  
  // printf("进程ID = %d\n", process.pid);
  // printf("进程名 = %s\n", process.name);
  // printf("父进程PID = %d\n",process.ppid);   
  // addNode(process.pid, process.ppid, process.name);

  if(fp)fclose(fp);
  
  free(process.name);
  
  return process.ppid;
}


int isNumeric(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

int cmp(const void * a, const void * b) {return ( *(int*)a - *(int*)b );}

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

void exe_n(int argc, char *argv[]) {
  printf("----exe_n----\n");
}

void exe_V(int argc, char*argv[]){printf("pstree-32/64 (OS2024 - Ziyan Shi) version 0.0.1\nCopyright (C) 2024-2024 NJU and Ziyan Shi\nPSmisc comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it under the terms of the GNU General Public License.\nFor more information about these matters, see the files named COPYING\n");}

void exe_root(int argc, char *argv[]){
  psNode *root = NULL;
  //扫描/proc目录，获取所有进程的PID
  //并且按照PID的大小进行排序
  //然后构建进程树
  //最后输出进程树

  
  printf("argc = %d\n", argc);
  int targetPID;
  if(argc<3){
    targetPID = 1;
    printf("No targetPID, use default PID = 1\n");
  }else{//argc>=3时，argv[2]才不是一个空指针（0x0）
    targetPID = atoi(argv[2]);
    //atoi函数试图将字符串转换为整数，
    //但是这里的argv[2]是一个空指针（0x0），
    //导致SIGSEGV错误。
  }
    
  printf("root:%d\n",targetPID);
  printf("-----try to open /proc/*-----\n");
  DIR *dir;
  struct dirent *entry;
  int count = 0;
  dir = opendir("/proc/");
  if(dir == NULL){perror("opendir error");return;}
  count = 1;
  int pid;
  int *pids = (int*)malloc(1000*sizeof(int));
  pids[0] = 1;

  int ppid;
  while((entry = readdir(dir)) != NULL){
    if(isNumeric(entry->d_name)){
      pid = atoi(entry->d_name);
      pids[count++] = pid;
      printf("pid = %d\n", pid);  
      psNode *p = getNode(pid, root);
      if(!p){
        ppid = getPPID(pid);
        if(ppid == -1)continue;
        addNode(pid, ppid, entry->d_name);
      }
      //打印p的信息
      if(p){
        printf("pid = %d, ppid = %d, name = %s\n", p->pid, p->ppid, p->name);
      }
      
      // ppid = getPPID(pid); 
      // if(ppid == -1)continue;
      // addNode(pid, ppid, entry->d_name);
      // printf(" %5d-%5d \n", pid,ppid);
    }
    entry = readdir(dir);
  }
  puts("");
  printf("count = %d\n", count);
  
  qsort(pids,count,sizeof(int),cmp);// function well
  
  // print all of the PIDs
  // printf("PIDs : ");
  // for(int i=0;i<count;i++){
  //   printf("%d ",pids[i]);
  // }
  // puts("");

  if(dir)closedir(dir);
  
}


int main(int argc, char *argv[]) {

  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
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
    printf("optind = %d\n", optind); 
    printf("No targetPID\n");
    exe_root(argc,argv);
  }
  
  assert(!argv[argc]);//确保命令行参数列表以空指针结尾，如果不是，则会触发断言错误。
  return 0;
}




/*
tyeodef struct{
  int pid;
  psNode *Parent;
  psNode *FirstSon;
  psNode *NextSubling;
}psNode;

class psTree{
  Process *root;
public:
  psTree(int pid){
    root.pid = pid;
    root->Parent = NULL;
    root->FirstSon = NULL;
    root->NextSubling = NULL;
  }
  ~psTree(){}
  psNode* getNode(int pid, psNode *p){
    if(!p)return NULL;
    if(p.pid==pid)return p;
    psNode* targetNode=NULL;
    targetNode = getNode(pid,p->FirstSon);
    if(!targetNode){
      targetNode = getNode(pid,p->NextSubling);
    }
    return targetNode;
  }
  void Insert(int ppid,int pid){
    psNode *p = getNode(ppid);
    psNode *q = new psNode(pid);
    q->Parent = p;
    if(!p->FirstSon){
      p->FirstSon = q;
    }else{
      psNode *t = p->FirstSon;
      while(t->NextSubling){
        t = t->NextSubling;
      }
      t->NextSubling = q;
    }
  }
};


*/