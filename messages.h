#ifndef MESSAGES_H
#define MESSAGES_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Data Structures */
typedef struct Node {
    char message[1024];
    struct Node* next;
} Node;

typedef struct LinkedList {
    Node* head;
    pthread_mutex_t lock;
} LinkedList;

/* Basic List Operations */
void listInit(LinkedList* list);
void listDestroy(LinkedList* list);
void listInsert(LinkedList* list, const char* message);
void listDelete(LinkedList* list, const char* message);
int listSearch(LinkedList* list, const char* message);
void listPrint(const LinkedList* list);
void listClear(LinkedList* list);

/* Specific Print Operations */
void listPrintStatus(const LinkedList* list);
void listPrintGroups(const LinkedList* list);
void listGetOnline(const LinkedList* list, LinkedList* onlineList, const char* username);

#ifdef __cplusplus
}
#endif

#endif // MESSAGES_H