#ifndef PUBLISHER_H 
#define PUBLISHER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Constants */
#define ADDRESS_P     "tcp://localhost:1883"
#define QOS_P         2
#define TIMEOUT_P     10000L

/* Core Functions */
int publisherRetained(const char* username_p, const char* topic_p, const char* payload_p);

/* Higher Level Functions */
void setStatus(const char* username, const char* status);
void setGroup(const char* groupname, const char* username);

#ifdef __cplusplus
}
#endif

#endif // PUBLISHER_H