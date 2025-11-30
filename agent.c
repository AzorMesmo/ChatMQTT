// Imports

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTAsync.h"
#include "constants.h"
#include "messages.h"
#include "subscriber.h"

#if !defined(_WIN32)
#include <unistd.h>
#else
#include <windows.h>
#endif

#if defined(_WRS_KERNEL)
#include <OsWrapper.h>
#endif

// Context

typedef struct 
{
    MQTTAsync client;
    char username_a[64];
    char topic_a[64];
    LinkedList* message_list;
    volatile int* online;
} Context_a;

// Flags

int finished_disc = 0; // Disconnection Finished
int is_subscribed = 0; // Subscription Successful
int finished_agent = 0; // Program Finished

// Function Prototypes

void onConnect_a(void* context_, MQTTAsync_successData* response);
void onConnectFailure_a(void* context_, MQTTAsync_failureData* response);
void connectionLost_a(void *context_, char *cause);
int messageArrived_a(void *context_, char *topicName, int topicLen, MQTTAsync_message *message);
void onDisconnectFailure_a(void* context_, MQTTAsync_failureData* response);
void onDisconnect_a(void* context_, MQTTAsync_successData* response);
void onSubscribe_a(void* context_, MQTTAsync_successData* response);
void onSubscribeFailure_a(void* context_, MQTTAsync_failureData* response);
int subscriberRetained(const char* username_a, const char* topic_a, LinkedList* status_list);
void updateGroup(const char* payload);
void processGroupRequest(const char* payload, const char* username);
void processGroupResponse(const char* payload, const char* username);



// Callbacks

void connectionLost_a(void *context_, char *cause) // Connection Lost
{
	Context_a* context = (Context_a*)context_;
	MQTTAsync client = context->client; // Cast context Back To The Original Type (void* -> MQTTAsync) To Be Able To Use
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	int rc;

    if (LOG_ENABLED)
    {
        printf("\n               [LOG] AGENT: Connection lost\n");
        if (cause)
            printf("     cause: %s\n", cause);
        printf("               [LOG] AGENT: Reconnecting\n");
    }

	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 0;
	conn_opts.onSuccess = onConnect_a;
	conn_opts.onFailure = onConnectFailure_a;
	conn_opts.context = context;

	// Try To Connect Again
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("AGENT: Failed to start connect, return code %d\n", rc);
		finished_agent = 1;
	}
}

void publishMessage(MQTTAsync client, const char* topic, const char* payload, int retained)
{
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer; // Response Options (... = [Default Initializer Macro])
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer; // Connection Options (... = [Default Initializer Macro])

    pubmsg.payload = (void*)payload;
    pubmsg.payloadlen = (int)strlen(payload);
    pubmsg.qos = QOS;
    pubmsg.retained = retained;

    int rc;
    if ((rc = MQTTAsync_sendMessage(client, topic, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
    {
        printf("               [LOG] AGENT: Failed to start sendMessage, return code %d\n", rc);
    }
    else if (LOG_ENABLED)
    {
        printf("               [LOG] AGENT: Published message \"%s\" to topic \"%s\"\n", payload, topic);
    }
}

int messageArrived_a(void *context_, char *topic_name, int topic_len, MQTTAsync_message *message) // Message Arrived
{
    Context_a* context = (Context_a*)context_;

    // Ensure Payload Is Nul-Terminated

    char buf[1024];
    int len = message->payloadlen;
    if (len >= (int)sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, message->payload, len);
    buf[len] = '\0';

    if (LOG_ENABLED)
    {
    printf("\n               [LOG] AGENT: Message arrived\n");
    printf("               [LOG]      Topic: %s\n", topic_name);
    printf("               [LOG]    Message: %s\n\n", buf);
    }

    char reply_topic[2048];

    // Message Type
    
    if (strstr(buf, "USER_REQUEST") != NULL) // User Conversation Request | USER_REQUEST:[USERNAME]
    { 
        if (LOG_ENABLED)
            printf("               [LOG] AGENT: User request received. %s\n", buf);

        snprintf(reply_topic, sizeof(reply_topic), "%s/REQUESTS/%s", topic_name, buf); // [USER]_Control/REQUESTS/[REQUEST_BODY]

        publishMessage(context->client, reply_topic, buf, 1);
    }
    else if (strstr(buf, "GROUP_REQUEST") != NULL) // Group Conversation Request | GROUP_REQUEST:[GROUPNAME];[USERNAME]
    {
        if (LOG_ENABLED)
            printf("               [LOG] AGENT: Group request received. %s\n", buf);
        snprintf(reply_topic, sizeof(reply_topic), "%s/REQUESTS/%s", topic_name, buf); // [USER]_Control/REQUESTS/[REQUEST_BODY]

        publishMessage(context->client, reply_topic, buf, 1);
    }
    else if (strstr(buf, "USER_ACCEPTED") != NULL) // User Conversation Accepted | USER_ACCEPTED:[USERNAME];[TOPIC]
    {
        if (LOG_ENABLED)
            printf("               [LOG] AGENT: User accept received. %s\n", buf);

        char* old_type = strtok(buf, ":");
        char* user = strtok(NULL, ";");
        char* link = strtok(NULL, ";");
        char new_type[512];
        snprintf(new_type, sizeof(new_type), "USER_REQUEST_ACCEPTED:%s;%s", user, link);
        snprintf(reply_topic, sizeof(reply_topic), "%s/HISTORY/%s", topic_name, new_type); // [USER]_Control/HISTORY/[REQUEST_BODY]
        
        publishMessage(context->client, reply_topic, new_type, 1);

        snprintf(reply_topic, sizeof(reply_topic), "CHATS/%s", link);
        // subscriberDirty(context->username_a, reply_topic, NULL);
        listInsert(context->message_list, reply_topic);

        // Confirm Conversation Topic Creation By Sending ""
        publishMessage(context->client, reply_topic, "", 1);
    }
    else if (strstr(buf, "GROUP_ACCEPTED") != NULL) // Group Conversation Accepted | GROUP_ACCEPTED:[GROUPNAME];[USERNAME];[TOPIC]
    {
        if (LOG_ENABLED)
            printf("               [LOG] AGENT: Group accept received. %s\n", buf);

        char* old_type = strtok(buf, ":");
        char* group = strtok(NULL, ";");
        char* user = strtok(NULL, ";");
        char* link = strtok(NULL, ";");
        char new_type[512];
        snprintf(new_type, sizeof(new_type), "GROUP_REQUEST_ACCEPTED:%s;%s;%s", group, user, link);
        snprintf(reply_topic, sizeof(reply_topic), "%s/HISTORY/%s", topic_name, new_type); // [USER]_Control/HISTORY/[REQUEST_BODY]
        
        publishMessage(context->client, reply_topic, new_type, 1);
    }
    else if (strstr(buf, "USER_REJECTED") != NULL) // User Conversation Rejected | USER_REJECTED:[USERNAME]
    {
        if (LOG_ENABLED)
            printf("               [LOG] AGENT: User reject received. %s\n", buf);

        char* old_type = strtok(buf, ":");
        char* user = strtok(NULL, ";");
        char new_type[512];
        snprintf(new_type, sizeof(new_type), "USER_REQUEST_REJECTED:%s", user);
        snprintf(reply_topic, sizeof(reply_topic), "%s/HISTORY/%s", topic_name, new_type); // [USER]_Control/HISTORY/[REQUEST_BODY]
        
        publishMessage(context->client, reply_topic, new_type, 1);
    }
    else if (strstr(buf, "GROUP_REJECTED") != NULL) // Group Conversation Rejected | GROUP_REJECTED:[GROUPNAME];[USERNAME]
    {
        if (LOG_ENABLED)
            printf("               [LOG] AGENT: Group reject received. %s\n", buf);

        char* old_type = strtok(buf, ":");
        char* group = strtok(NULL, ";");
        char* user = strtok(NULL, ";");
        char new_type[512];
        snprintf(new_type, sizeof(new_type), "GROUP_REQUEST_REJECTED:%s;%s", group, user);
        snprintf(reply_topic, sizeof(reply_topic), "%s/HISTORY/%s", topic_name, new_type); // [USER]_Control/HISTORY/[REQUEST_BODY]
        
        publishMessage(context->client, reply_topic, new_type, 1);
    }

	// Memory Management
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topic_name);

    return 1;
}

void onDisconnectFailure_a(void* context_, MQTTAsync_failureData* response) // Fails To Disconnect
{
    if (LOG_ENABLED)
        printf("               [LOG] AGENT: Disconnect failed, rc %d\n", response->code);
    finished_disc = 1;
}

void onDisconnect_a(void* context_, MQTTAsync_successData* response) // Disconnected Successfuly
{
    if (LOG_ENABLED)
        printf("               [LOG] AGENT: Successful disconnection\n");
    finished_disc = 1;
}

void onSubscribe_a(void* context_, MQTTAsync_successData* response) // Subscribed Successfuly
{
    if (LOG_ENABLED)
        printf("               [LOG] AGENT: Subscribe succeeded\n");
    is_subscribed = 1;
}

void onSubscribeFailure_a(void* context_, MQTTAsync_failureData* response) // Fails To Subscribe
{
    if (LOG_ENABLED)
        printf("               [LOG] AGENT: Subscribe failed, rc %d\n", response->code);
    finished_agent = 1;
}


void onConnectFailure_a(void* context_, MQTTAsync_failureData* response) // Fails To Connect
{
    if (LOG_ENABLED)
        printf("               [LOG] AGENT: Connect failed, rc %d\n", response->code);
    finished_agent = 1;
}


void onConnect_a(void* context_, MQTTAsync_successData* response) // Connected Successfuly
{
	Context_a* context = (Context_a*)context_;
	MQTTAsync client = context->client;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer; // Response Options (... = [Default Initializer Macro])
	int rc; // >main

    if (LOG_ENABLED)
    {
        printf("               [LOG] AGENT: Successful connection\n");
        printf("               [LOG] AGENT: Subscribing to topic %s for client %s using QoS-%d\n", context->topic_a, context->username_a, QOS);
    }

    // Connection Parameters
    opts.onSuccess = onSubscribe_a;
    opts.onFailure = onSubscribeFailure_a;
    opts.context = context;

    // Subscribe To Topic
    if ((rc = MQTTAsync_subscribe(client, context->topic_a, QOS, &opts)) != MQTTASYNC_SUCCESS)
    {
        if (LOG_ENABLED)
            printf("               [LOG] AGENT: Failed to start subscribe, return code %d\n", rc);
        finished_agent = 1;
    }
}

// Main Functions

int agentControl(const char* username_a, LinkedList* control_list, volatile int* online) // Subscribe To A Retained Topic (Not Mantain Connection)
{
	MQTTAsync client; // Client (Handler) | Connection To Broker
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer; // Disconnection Options (... = [Default Initializer Macro])
	int rc; // Return Code For Function Calls
	int ch; // Input Character Storage | int <-> getchar()

	// Reset Parameters

	finished_disc = 0;
	is_subscribed = 0;
	finished_agent = 0;

	// Create Client

    if ((rc = MQTTAsync_create(&client, ADDRESS, username_a, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
        if (LOG_ENABLED)
            printf("               [LOG] AGENT: Failed to create client, return code %d\n", rc);
        return EXIT_FAILURE;
    }

	// Create Context

    Context_a* context = malloc(sizeof(Context_a));
    if (!context) {
        MQTTAsync_destroy(&client);
        return EXIT_FAILURE;
    }

    context->client = client; // Client
    strcpy(context->username_a, username_a); // Username
    strcpy(context->topic_a, username_a); // Topic
    context->message_list = control_list; // Status (Message) List

    // Set Callbacks

    if ((rc = MQTTAsync_setCallbacks(client, context, connectionLost_a, messageArrived_a, NULL)) != MQTTASYNC_SUCCESS)
	{
        if (LOG_ENABLED)
            printf("               [LOG] AGENT: Failed to set callbacks, return code %d\n", rc);
        free(context);
        MQTTAsync_destroy(&client);
        return EXIT_FAILURE;
    }

	// Set Connection Parameters

	conn_opts.keepAliveInterval = 30;
	conn_opts.cleansession = 0; // Persistance
	conn_opts.onSuccess = onConnect_a; // Calls onConnect If Connect Successfuly
	conn_opts.onFailure = onConnectFailure_a; // Calls onConnectFailure If Connection Fails
	conn_opts.context = context;

	// Connect To Broker

    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
        if (LOG_ENABLED)
            printf("               [LOG] AGENT: Failed to start connect, return code %d\n", rc);
        free(context);
        MQTTAsync_destroy(&client);
        return EXIT_FAILURE;
    }

	// Wait For Subscription

    while (!is_subscribed && !finished_agent) {
        #if defined(_WIN32)
            Sleep(DELAY_100_MS_MS);
        #else
            usleep(DELAY_100_MS_US);
        #endif
    }

    while (*online) {
        #if defined(_WIN32)
            Sleep(DELAY_100_MS_MS);
        #else
            usleep(DELAY_100_MS_US);
        #endif
    }

    if (finished_agent) {
        if (LOG_ENABLED)
            printf("               [LOG] AGENT: Client Destroyed\n");
        MQTTAsync_destroy(&client);
        free(context);
        return EXIT_FAILURE;
    }

	// Disconnection Parameters

	disc_opts.onSuccess = onDisconnect_a;
	disc_opts.onFailure = onDisconnectFailure_a;

	// Disconnect To Broker

    if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS) {
        if (LOG_ENABLED)
            printf("               [LOG] AGENT: Failed to start disconnect, return code %d\n", rc);
        MQTTAsync_destroy(&client);
        free(context);
        return EXIT_FAILURE;
    }

	// Wait Disconnection

 	while (!finished_disc)
 	{
		#if defined(_WIN32)
            Sleep(DELAY_100_MS_MS);
        #else
            usleep(DELAY_100_MS_US);
        #endif
 	}

    MQTTAsync_destroy(&client);
    free(context);
    return rc;
}