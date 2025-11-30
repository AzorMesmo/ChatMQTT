#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include "messages.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constants */
#define ADDRESS_S     "tcp://localhost:1883"
#define QOS_S         2
#define TIMEOUT_S     10000L

/* Core Functions */
int subscriberRetained(const char* username_s, const char* topic_s, LinkedList* status_list);
int subscriberDirty(const char* username_s, const char* topic_s, LinkedList* status_list);
int subscriberConversation(const char* username_s, const char* topic_s,  LinkedList* message_list, volatile int* chatting);

/* Higher Level Functions */
void getUsers(const char* username, LinkedList* status_list, int print_status);
void getGroups(const char* username, LinkedList* groups_list, int print_groups);

#ifdef __cplusplus
}
#endif

#endif // SUBSCRIBER_H