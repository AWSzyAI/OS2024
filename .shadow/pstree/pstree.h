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


/*Log*/
static inline void printNode(psNode *node){
    if(!node){
        printf("[Log] Print No node\n");
        return;
    }
    printf("---printNode---\n");
    printf("pid = %d, name = %s, ppid = %d\n", node->pid, node->name, node->ppid);
    
}
static inline void printArray(int *arr, int n){
    printf("count = %d\n", n);
    for(int i=0;i<n;i++){
        printf("%d ",arr[i]);
    }
    puts("");
}
static inline void PrintTree(psNode *root, int depth){
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

static inline int GetRootPID(int argc, char *argv[]){
    int rootPID;
    if(argc<3){//无参数 ./pstree-64 
        rootPID = 1;
    }else{//argc>=3时，argv[2]才不是一个空指针（0x0）
        rootPID = atoi(argv[2]);
    }
    return rootPID;
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






static inline psNode * NewNode(int pid){
    printf("[Log] NewNode %d\n", pid);

    char filename[100];
    sprintf(filename, "/proc/%d/stat", pid);
    FILE *fp = fopen(filename, "r");
    if(!fp){
        printf("No such process\n");
        return NULL;
    }
    char line[MAX_LINE_LENGTH+1];
    fgets(line, MAX_LINE_LENGTH, fp);
    if(fp)fclose(fp);

    psNode *node = (psNode*)malloc(sizeof(psNode));
    
    printNode(node);
    printf("[Log] ! %d\n", pid);
    sscanf(line, "%d", &node->pid);//get PID
    char *token = strtok(line, " "); //跳过进程ID
    token = strtok(NULL, " ");
    // process.name = (char*)malloc(MAX_LINE_LENGTH * sizeof(char));
    sscanf(token, "%s", node->name);//get name
    token = strtok(NULL, " ");
    token = strtok(NULL, " ");
    sscanf(token, "%d", &node->ppid);//get PPID
    
    node->depth = 0;
    node->Parent = NULL;
    node->FirstSon = NULL;
    node->NextSibling = NULL;

    printf("进程ID = %d\n", node->pid);
    printf("进程名 = %s\n", node->name);
    printf("父进程PID = %d\n",node->ppid);

    
    return node;
}

static inline psNode * getNode(int pid, psNode *root){
    if(!root)return NULL;
    if(root->pid == pid)return root;
    psNode *node = getNode(pid, root->FirstSon);
    if(node)return node;
    return getNode(pid, root->NextSibling);
}



static inline psNode * addNewNode(int pid, psNode *root){
    if(!root){
        root = NewNode(pid);
        printNode(root);
        return root;
    }
    psNode *node = NewNode(pid);
    if(!node)return root;
    int ppid = node->ppid;

    psNode *parent = getNode(ppid, root);
    if(!parent){
        printf("No such parent process\n");
        return root;
    }
    node->depth = parent->depth + 1;
    node->Parent = parent;
    if(!parent->FirstSon){
        parent->FirstSon = node;
    }else{
        psNode *temp = parent->FirstSon;
        while(temp->NextSibling){
            temp = temp->NextSibling;
        }
        temp->NextSibling = node;
    }
    return root;
}


/*Done*/

static inline void readargs(int argc, char *argv[]){
    for (int i = 0; i < argc; i++) {
        assert(argv[i]);
        printf("argv[%d] = %s\n", i, argv[i]);
    }
}







#endif // PSTREE_H