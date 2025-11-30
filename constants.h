// MQTT Parameters
#define ADDRESS     "tcp://localhost:1883" // Default: "tcp://test.mosquitto.org:1883"
#define QOS 2

// Parameters
#define MAX_GROUP_MEMBERS 50

// Time Delays
#define DELAY_100_MS_MS 100
#define DELAY_100_MS_US 100000L
#define DELAY_5_SEC_MS 5000
#define DELAY_5_SEC_US 5000000L
#define DELAY_10_SEC_MS 10000
#define DELAY_10_SEC_US 10000000L

// Log
#define LOG_ENABLED 0

// Main Topics
// USERS/  > User Status   ([USER]:[STATUS])
// GROUPS/ > Groups        ([GROUP_NAME]:[LEADER]:[MEMBER1];[MEMBER2];...])
// CHATS/  > Conversations ([USER]_[USER]|[TIMESTAMP] / [GROUPNAME]|[TIMESTAMP])
// [USER]_Control/ > Control Topic (Control Message Handler - agent.c)
//               /REQUESTS/ > Conversation Requests ([REQUEST_TYPE]/[REQUEST_BODY])
//               /HISTORY/  > Events History        ([EVENT_TYPE]/[EVENT_BODY])

// Possible Request Type Received By Topic
// --- X_Control/ ---
// USER_REQUEST:[USERNAME]                               | [USERNAME]  > Your User
// GROUP_REQUEST:[GROUPNAME];[USERNAME]                  | [GROUPNAME] > My Group       / [USERNAME] > Your User
// USER_ACCEPTED:[USERNAME];[TOPIC]                      | [USERNAME]  > Your User      / [TOPIC]    > "RECEIVER|SENDER|TIMESTAMP"
// GROUP_ACCEPTED:[GROUPNAME];[USERNAME];[TOPIC]         | [GROUPNAME] > Your Group     / [USERNAME] > Your User                 / [TOPIC] > "GROUPNAME|TIMESTAMP"
// USER_REJECTED:[USERNAME]                              | [USERNAME]  > Your User
// GROUP_REJECTED:[GROUPNAME];[USERNAME]                 | [GROUPNAME] > Your Group     / [USERNAME] > Your User
// --- REQUESTS/ ---
// USER_REQUEST:[USERNAME]                               | [USERNAME]  > Your User
// GROUP_REQUEST:[GROUPNAME];[USERNAME]                  | [GROUPNAME] > My Group       / [USERNAME] > Your User
// --- HISTORY/ ---
// GROUP_CREATED:[GROUPNAME];[TOPIC]                     | [GROUPNAME] > My Group       / [TOPIC]    > "GROUPNAME|TIMESTAMP"
// USER_REQUEST:[USERNAME]                               | [USERNAME]  > Your User
// GROUP_REQUEST:[GROUPNAME];[USERNAME]                  | [GROUPNAME] > Your Group     / [USERNAME] > Your User (Leader)
// USER_REQUEST_ACCEPTED:[USERNAME];[TOPIC]              | [USERNAME]  > Your User
// GROUP_REQUEST_ACCEPTED:[GROUPNAME];[USERNAME];[TOPIC] | [GROUPNAME] > Your Group     / [USERNAME] > Your User (Leader)
// USER_REQUEST_REJECTED:[USERNAME]                      | [USERNAME]  > Your User
// GROUP_REQUEST_REJECTED:[GROUPNAME];[USERNAME]         | [GROUPNAME] > Your Group     / [USERNAME] > Your User (Leader)
// USER_ACCEPTED:[USERNAME];[TOPIC]                      | [USERNAME]  > Your User      / [TOPIC]    > "RECEIVER|SENDER|TIMESTAMP"
// GROUP_ACCEPTED:[GROUPNAME];[USERNAME]                 | [GROUPNAME] > Your Group     / [USERNAME] > Your User                 / [TOPIC] > "GROUPNAME|TIMESTAMP"
// USER_REJECTED:[USERNAME]                              | [USERNAME]  > Your User
// GROUP_REJECTED:[GROUPNAME];[USERNAME]                 | [GROUPNAME] > Your Group     / [USERNAME] > Your User

// Request Type Tranformation
// X_Control / USER_REQUEST   -> REQUESTS / USER_REQUEST 
// X_Control / GROUP_REQUEST  -> REQUESTS / GROUP_REQUEST
// X_Control / USER_ACCEPTED  -> HISTORY  / USER_REQUEST_ACCEPTED
// X_Control / GROUP_ACCEPTED -> HISTORY  / GROUP_REQUEST_ACCEPTED
// X_Control / USER_REJECTED  -> HISTORY  / USER_REQUEST_REJECTED
// X_Control / GROUP_REJECTED -> HISTORY  / GROUP_REQUEST_REJECTED