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
#define LOG_ENABLED 1

// CONTROL
//
// REQUESTS
// USER_REQUEST:[USERNAME]                       | [USERNAME] > Sender User
// GROUP_REQUEST:[GROUPNAME];[USERNAME]          | [GROUPNAME] > Target Group / [USERNAME] > Sender User
//
// RESPONSES
// USER_ACCEPTED:[USERNAME];[TOPIC]              | [USERNAME] > Sender User / [TOPIC] > "RECEIVER|SENDER|TIMESTAMP"
// GROUP_ACCEPTED:[GROUPNAME];[USERNAME];[TOPIC] | [GROUPNAME] > Target Group / [USERNAME] > Sender User / [TOPIC] > "GROUPNAME|TIMESTAMP"
// USER_REJECTED:[USERNAME]                      | [USERNAME] > Sender User
// GROUP_REJECTED:[GROUPNAME];[USERNAME]         | [GROUPNAME] > Target Group / [USERNAME] > Sender User
//
// CONFIRMATIONS
// GROUP_CREATED:[GROUPNAME];[TOPIC]             | [GROUPNAME] > Created Group / [TOPIC] > "GROUPNAME|TIMESTAMP"
// USER_REQUEST_SENT:[USERNAME]                  | [USERNAME] > Receiver User
// GROUP_REQUEST_SENT:[GROUPNAME];[USERNAME]     | [GROUPNAME] > Target Group / [USERNAME] > Receiver User (Leader)
