#ifndef PSTREE_H
#define PSTREE_H

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



static inline void PrintTree(psNode *root, int depth){
}



static inline int getPPID(int targetPID){
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


static inline int isNumeric(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isdigit(str[i])) {
            return 0;
        }
    }
    return 1;
}

int cmp(const void * a, const void * b) {return ( *(int*)a - *(int*)b );}


static inline void exe_n(int argc, char *argv[]) {
    printf("----exe_n----\n");
}

static inline void exe_V(int argc, char*argv[]){printf("pstree-32/64 (OS2024 - Ziyan Shi) version 0.0.1\nCopyright (C) 2024-2024 NJU and Ziyan Shi\nPSmisc comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it under the terms of the GNU General Public License.\nFor more information about these matters, see the files named COPYING\n");}


static inline int GetRootPID(int argc, char *argv[]){
    int rootPID;
    if(argc<3){//无参数 ./pstree-64 
        rootPID = 1;
    }else{//argc>=3时，argv[2]才不是一个空指针（0x0）
        rootPID = atoi(argv[2]);
    }
    return rootPID;
}

static inline void printArray(int *arr, int n){
    printf("count = %d\n", n);
    for(int i=0;i<n;i++){
        printf("%d ",arr[i]);
    }
    puts("");
}

static inline int getPIDs(int *pids){
    /*
    -1 代表失败
    */
    DIR *dir;
    struct dirent *entry;
    dir = opendir("/proc/");
    if(dir == NULL){perror("opendir error");return -1;}

    int count = 1;
    int pid,ppid;
    
    pids[0] = 1;

    while((entry = readdir(dir)) != NULL){
        if(isNumeric(entry->d_name)){
        pid = atoi(entry->d_name);
        pids[count++] = pid;
        }
        entry = readdir(dir);
    }
    puts("");
    qsort(pids,count,sizeof(int),cmp);// function well
    if(dir)closedir(dir);
    return count;
}

psNode * addNewNode(int pid, psNode *root){
    if(!root){
        root = (psNode*)malloc(sizeof(psNode));
        root->pid = pid;
        root->ppid = getPPID(pid);
        root->name = "root";
        root->depth = 0;
        root->Parent = NULL;
        root->FirstSon = NULL;
        root->NextSibling = NULL;
    }
    return NULL;
}

static inline void cmd_root(int argc, char *argv[]){
    //读取参数，定义root PID
    int rootPID = GetRootPID(argc,argv);
    printf("[Log] rootPID = %d\n", rootPID);//[ ] Log系统有待优化
    
    //扫描/proc目录，获取所有进程的PID
    
    int *pids = (int*)malloc(1000*sizeof(int));
    int cntPIDs =  getPIDs(pids);
    printArray(pids,cntPIDs);

    //构建进程树
    psNode *root = NULL;
    // for(int i=0;i<cntPIDs;i++)root = addNewNode(pids[i], root);
    //最后输出进程树
    // PrintTree(root, 0);
    //释放内存
    free(pids);
}


static inline void readargs(int argc, char *argv[]){
    for (int i = 0; i < argc; i++) {
        assert(argv[i]);
        printf("argv[%d] = %s\n", i, argv[i]);
    }
}

#endif // PSTREE_H