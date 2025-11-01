#ifndef SUBSCRIBER_H
#define SUBSCRIBER_H

#include "messages.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ADDRESS_S     "tcp://localhost:1883"
#define QOS_S         2
#define TIMEOUT_S     10000L
#define SUBSCRIBER_STATUS_TIMEOUT 40

/* Subscribe to users status updates */
int subscriberUsers(const char* username_s, const char* topic_s, LinkedList* status_list);

/* Subscribe to groups updates */
int subscriberGroups(const char* username_s, const char* topic_s, LinkedList* groups_list);

/* Message processing functions */
void processGroupRequest(const char* payload, const char* username);
void processGroupResponse(const char* payload, const char* username); 
void updateGroup(const char* payload);

#ifdef __cplusplus
}
#endif

#endif // SUBSCRIBER_H