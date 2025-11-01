#ifndef PUBLISHER_H 
#define PUBLISHER_H

#ifdef __cplusplus
extern "C" {
#endif

#define ADDRESS_P     "tcp://localhost:1883"
#define QOS_P         2
#define TIMEOUT_P     10000L

/* Publish user status */
int publisherStatus(const char* username_p, const char* topic_p, const char* payload_p);

/* Publish group updates */
int publisherGroups(const char* username_p, const char* topic_p, const char* payload_p);

#ifdef __cplusplus
}
#endif

#endif // PUBLISHER_H