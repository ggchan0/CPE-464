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

ClientNode *initializeClientNode(char *handle, int socketNum);

Nodelist *initializeNodelist();

void freeNodelist(Nodelist *list);

void removeNode(Nodelist *list, int socketNum);
