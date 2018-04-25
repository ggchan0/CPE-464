#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct ClientNode {
   char *handle;
   int socketNum;
   struct ClientNode *next;
} ClientNode;

typedef struct Nodelist {
   struct ClientNode *head;
} Nodelist;

void addToNodelist(Nodelist *nodelist, ClientNode *node);

ClientNode *initializeClientNode(int socketNum);

Nodelist *initializeNodelist();

void freeNode(ClientNode *temp);

void freeNodelist(Nodelist *list);

ClientNode *findNode(Nodelist *list, int socketNum);

void removeNode(Nodelist *list, int socketNum);
