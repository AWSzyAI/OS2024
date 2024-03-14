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

// Then, when you create a new variable of type psNode, you can initialize it:
// psNode node = { -1, NULL, -1, -1, NULL, NULL, NULL };


/*Log*/
static inline void printNode(psNode *node){
    if(!node){
        printf("[Log] Print No node\n");
        return;
    }
    printf("---printNode---\n");
    int pid = node->pid;
    char *name = node->name;
    int ppid = node->ppid;
    int depth = node->depth;
    printf("pid = %d, name = %s, ppid = %d, depth = %d\n", pid, name, ppid, depth);
    // printf("pid = %d, name = %s, ppid = %d\n", node->pid, node->name, node->ppid);
    
}
static inline void printArray(int **arr, int n){
    printf("count = %d\n", n);
    for(int i=0;i<n;i++){
        printf("%d %d\n", arr[i][0], arr[i][1]);
    }
    printf("----------------------\n");
    puts("");
}
static inline void PrintTree(psNode *root, int depth){
    if(!root)return;
    for(int i=0;i<root->depth;i++)printf("     ");
    printf("%s(%d) - %d\n", root->name,root->pid, root->depth);
    PrintTree(root->FirstSon, depth+1);
    PrintTree(root->NextSibling, depth);
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
static inline int getPIDs(int **pids){
    /*
    -1 代表失败
    */
    DIR *dir;
    struct dirent *entry;
    dir = opendir("/proc/");
    if(dir == NULL){perror("opendir error");return -1;}

    int count = 1;
    int pid,ppid;
    
    pids[0][0] = 1;
    pids[0][1] = 0;

    while((entry = readdir(dir)) != NULL){
        if(isNumeric(entry->d_name)){
            pid  = atoi(entry->d_name);
            ppid = getPPID(pid);
            // printf("%d %d\n", pid, ppid);
            pids[count][0] = pid;
            pids[count][1] = ppid;
            printf("%d %d\n", pids[count][0], pids[count][1]);
            count++;
        }
        entry = readdir(dir);
    }
    puts("");
    qsort(pids,count,sizeof(int),cmp);// function well
    if(dir)closedir(dir);
    return count;
}






static inline psNode * NewNode(int pid){
    // printf("[Log] NewNode %d\n", pid);

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
    
    //刚分配的node，不存在parent，firstSon，nextSibling，都是随机指针，打印会溢出
    // printNode(node);
    // printf("[Log] ! %d\n", pid);

    sscanf(line, "%d", &node->pid);//get PID
    char *token = strtok(line, " "); //跳过进程ID
    token = strtok(NULL, " ");
    char *name = (char*)malloc(MAX_LINE_LENGTH * sizeof(char));
    sscanf(token, "%s", name);//get name
    
    char *start = name;
    char *end = name + strlen(name) - 1;

    // Remove '(' from the start
    if (*start == '(') {
        start++;
    }

    // Remove ')' from the end
    if (*end == ')') {
        *end = '\0';
    }

    name = start;
    node->name = name;
    token = strtok(NULL, " ");
    token = strtok(NULL, " ");
    sscanf(token, "%d", &node->ppid);//get PPID
    node->depth = 0;
    node->Parent = NULL;
    node->FirstSon = NULL;
    node->NextSibling = NULL;

    // printf("进程ID = %d\n", node->pid);
    // printf("进程名 = %s\n", node->name);
    // printf("父进程PID = %d\n",node->ppid);

    
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
    if(!node){
        printf("[Log] newnode fail");
        return root;
    }

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



static inline void ConstructTree(psNode *p, int **pids, int cntPIDs, int pid){

    for(int i=0;i<cntPIDs;i++){
        if(pids[i][1] == pid){
            psNode *q = NewNode(pids[i][0]);
            q->Parent = p;
            q->depth = p->depth + 1;
            if(!p->FirstSon){
                p->FirstSon = q;
            }else{
                psNode *temp = p->FirstSon;
                while(temp->NextSibling){
                    temp = temp->NextSibling;
                }
                temp->NextSibling = q;
            }
            ConstructTree(q, pids, cntPIDs, pids[i][0]);
        }
    }

}


/*Done*/

static inline void readargs(int argc, char *argv[]){
    for (int i = 0; i < argc; i++) {
        assert(argv[i]);
        printf("argv[%d] = %s\n", i, argv[i]);
    }
}







#endif // PSTREE_H