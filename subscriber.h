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

/* Subscribe to a topic and collect status messages into status_list.
   Use const char* for string arguments. */
int subscriberStatus(const char* username_p, const char* topic_p, LinkedList* status_list);

#ifdef __cplusplus
}
#endif

#endif // SUBSCRIBER_H