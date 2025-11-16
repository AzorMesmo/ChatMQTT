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
void toUppercase(char *str);

/* Specific Print Operations */
void listPrintStatus(const LinkedList* list);
void listPrintGroups(const LinkedList* list);
void listPrintRequests(const LinkedList* list);
void listPrintHistory(const LinkedList* list, const char *username);
void listGetOnlineUsers(const LinkedList* list, LinkedList* onlineList, const char* username);
void listGetOnlineGroups(LinkedList* online_users_list, const LinkedList* groups_list, LinkedList* online_list, const char* username);
char* listGetGroupLeader(const LinkedList* groups_list, const char* group);
int listSearchFirstParameter(LinkedList* list, const char* message);

#ifdef __cplusplus
}
#endif

#endif // MESSAGES_H