#ifndef PSTACK_H
#define PSTACK_H

#include "psNode.h"

#define INITAL_STACK_SIZE 10

typedef struct{
    psNode **data;
    int top;
    int size;
}Stack;

Stack *initStack();
void push(Stack *s, psNode *node);
psNode *pop(Stack *s);
int isEmpty(Stack *s);
void deleteStack(Stack *s);


Stack *initStack(){
    Stack *s = (Stack*)malloc(sizeof(Stack));
    s->data = (psNode**)malloc(INITAL_STACK_SIZE*sizeof(psNode*));
    s->top = -1;
    s->size = INITAL_STACK_SIZE;
    return s;
}

void push(Stack *s, psNode *node){
    if(s->top == s->size-1){
        s->size *= 2;
        s->data = (psNode**)realloc(s->data, s->size*sizeof(psNode*));
    }
    s->data[++s->top] = node;
}

psNode *pop(Stack *s){
    if(s->top == -1)return NULL;
    return s->data[s->top--];
}

int isEmpty(Stack *s){
    return s->top == -1;
}

void deleteStack(Stack *s){
    free(s->data);
    free(s);
}



#endif
// Path: OS2024/os-workbench/pstree/pstack.c
// Compare this snippet from OS2024/os-workbench/pstree/pstree.c:
// #include "pstack.h"
//
