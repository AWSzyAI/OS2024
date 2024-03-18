#include "pstree.h"



void deleteNode(psNode *root){
    if(!root)return;
    deleteNode(root->FirstSon);
    deleteNode(root->NextSibling);
    free(root);
}

static inline void cmd_root(int argc, char *argv[]){
    //读取参数，定义root PID
    int rootPID = GetRootPID(argc,argv);
    // get all PIDs
    int **pids = (int**)malloc(1000*sizeof(int*));
    for(int i=0;i<1000;i++){
        pids[i] = (int*)malloc(2*sizeof(int));
    }
    int cntPIDs =  getPIDs(pids);
    qsort(pids,cntPIDs,sizeof(int)*2,cmp_pid);// function well
    // printf("cntPIDs: %d\n",cntPIDs);
    // printArray(pids,cntPIDs);
    //构建进程树
    psNode *root = NULL;
    // printNode(root);
    // printf("[Log] pids[0] = %d\n", pids[0]);
    root = addNewNode(rootPID, root);
    ConstructTree_name(root, pids, cntPIDs, rootPID);
    // for(int i=0;i<cntPIDs;i++)root = addNewNode(pids[i], root);
    //最后输出进程树
    PrintTree(rootPID,root);
    //释放内存
    free(pids);
    deleteNode(root);
}
static inline void exe_n(int argc, char *argv[]){
    int rootPID = GetRootPID(argc,argv);
    int **pids = (int**)malloc(1000*sizeof(int*));
    for(int i=0;i<1000;i++){
        pids[i] = (int*)malloc(2*sizeof(int));
    }
    int cntPIDs =  getPIDs(pids);
    qsort(pids,cntPIDs,sizeof(int)*2,cmp_pid);// function well
    psNode *root = NULL;
    root = addNewNode(rootPID, root);
    ConstructTree(root, pids, cntPIDs, rootPID);
    PrintTree(rootPID,root);
    free(pids);
    deleteNode(root);
}
static inline void exe_V(int argc, char*argv[]){printf("pstree-32/64 (OS2024 - Ziyan Shi) version 0.0.1\nCopyright (C) 2024-2024 NJU and Ziyan Shi\nPSmisc comes with ABSOLUTELY NO WARRANTY.\nThis is free software, and you are welcome to redistribute it under the terms of the GNU General Public License.\nFor more information about these matters, see the files named COPYING\n");}
static inline void exe_p(int argc, char *argv[]){
    int rootPID = GetRootPID(argc,argv);
    int **pids = (int**)malloc(1000*sizeof(int*));
    for(int i=0;i<1000;i++){
        pids[i] = (int*)malloc(2*sizeof(int));
    }
    int cntPIDs =  getPIDs(pids);
    qsort(pids,cntPIDs,sizeof(int)*2,cmp_pid);// function well
    psNode *root = NULL;
    root = addNewNode(rootPID, root);
    ConstructTree(root, pids, cntPIDs, rootPID);
    PrintTree_p(rootPID,root);
    free(pids);
    deleteNode(root);
}
static inline void cmd(int argc, char *argv[]) {
    int opt;
    int option_processed = 0; // 标志变量
    while((opt=getopt(argc,argv,"npV"))!=-1){
        switch (opt)
        {
        case 'n':
        exe_n(argc, argv);
        option_processed = 1; // 设置标志变量
        break;
        case 'p':
        exe_p(argc, argv);
        option_processed = 1; // 设置标志变量
        break;
        // case 'V':
        // exe_V(argc, argv);
        // option_processed = 1; // 设置标志变量
        // break;
        case '?': // Add a case for '?' to handle unknown options
            if (strcmp(argv[optind - 1], "-V") == 0 || strcmp(argv[optind - 1], "--version") == 0) {
                exe_V(argc, argv);
                option_processed = 1;
            } else {
                printf("<usage> pstree [-npV]\n");
                option_processed = 1;
            }
            break;  
        default:
        printf("<usage> pstree [-npV]\n");
        option_processed = 1; // 设置标志变量
        break;
        }
    }

    if(!option_processed){
        cmd_root(argc,argv);
    }
}

int main(int argc, char *argv[]) {
    readargs(argc, argv);
    cmd(argc, argv);
    assert(!argv[argc]);//确保命令行参数列表以空指针结尾，如果不是，则会触发断言错误。
    return 0;
}

