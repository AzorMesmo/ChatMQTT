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
char* listPopLast(LinkedList* list);
void listPopPrintAll(LinkedList* list);
void listDelete(LinkedList* list, const char* message);
int listSearch(LinkedList* list, const char* message);
void listPrint(const LinkedList* list);
void listClear(LinkedList* list);
void toUppercase(char *str);

/* Specific Print Operations */
void listPrintStatus(const LinkedList* list);
void listPrintGroups(const LinkedList* list);
void listPrintRequests(const LinkedList* list);
void listPrintHistory(const LinkedList* list, const char *username);
void listPrintChats(const LinkedList* list, const char *username);
char* listGetGroup(const LinkedList* groups_list, const char* group, const char* leader);
char* listGetGroupLeader(const LinkedList* groups_list, const char* group);
char* listGetTopic(const LinkedList* history_list, const char* target);
int listSearchFirstParameter(LinkedList* list, const char* message);
int listSearchConversation(LinkedList* list, const char* target);
int listSearchChat(LinkedList* list);

#ifdef __cplusplus
}
#endif

#endif // MESSAGES_H