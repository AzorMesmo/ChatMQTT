// Imports

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTAsync.h"
#include "messages.h"
#include "constants.h"

#if !defined(_WIN32)
#include <unistd.h>
#else
#include <windows.h>
#endif

#if defined(_WRS_KERNEL)
#include <OsWrapper.h>
#endif

// Parameters
#define ADDRESS_S     "tcp://localhost:1883" // Default: "tcp://test.mosquitto.org:1883"
#define QOS_S         2

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
int finished_sub = 0; // Program Finished

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
	int rc; // >main

	printf("\n               [LOG] Agent: Connection lost\n");
	if (cause)
		printf("     cause: %s\n", cause);
	printf("               [LOG] Agent: Reconnecting\n");

	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect_a;
	conn_opts.onFailure = onConnectFailure_a;
	conn_opts.context = context;

	// Try To Connect Again
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Agent: Failed to start connect, return code %d\n", rc);
		finished_sub = 1;
	}
}

int messageArrived_a(void *context_, char *topicName, int topicLen, MQTTAsync_message *message) // Message Arrived
{
    Context_a* context = (Context_a*)context_;

    // Ensure Payload Is Nul-Terminated

    char buf[1024];
    int len = message->payloadlen;
    if (len >= (int)sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, message->payload, len);
    buf[len] = '\0';

    printf("\n               [LOG] Agent: Message arrived\n");
    printf("                  [LOG]   Topic: %s\n", topicName);
    printf("                  [LOG] Message: %s\n\n", buf);

    // Message Type

    if (strstr(buf, "USER") != NULL) // USER_REQUEST
    { 
        printf("               [LOG] Agent: User request received. %s\n", buf);
        listInsert(context->message_list, buf);
    }
    else if (strstr(buf, "GROUP") != NULL) // GROUP_REQUEST
    {
        printf("               [LOG] Agent: Group request received. %s\n", buf);
        listInsert(context->message_list, buf);
    }

	// Memory Management
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);

    return 1;
}

void onDisconnectFailure_a(void* context_, MQTTAsync_failureData* response) // Fails To Disconnect
{
    printf("               [LOG] Agent: Disconnect failed, rc %d\n", response->code);
    finished_disc = 1;
}

void onDisconnect_a(void* context_, MQTTAsync_successData* response) // Disconnected Successfuly
{
    printf("               [LOG] Agent: Successful disconnection\n");
    finished_disc = 1;
}

void onSubscribe_a(void* context_, MQTTAsync_successData* response) // Subscribed Successfuly
{
    printf("               [LOG] Agent: Subscribe succeeded\n");
    is_subscribed = 1;
}

void onSubscribeFailure_a(void* context_, MQTTAsync_failureData* response) // Fails To Subscribe
{
    printf("               [LOG] Agent: Subscribe failed, rc %d\n", response->code);
    finished_sub = 1;
}


void onConnectFailure_a(void* context_, MQTTAsync_failureData* response) // Fails To Connect
{
    printf("               [LOG] Agent: Connect failed, rc %d\n", response->code);
    finished_sub = 1;
}


void onConnect_a(void* context_, MQTTAsync_successData* response) // Connected Successfuly
{
	Context_a* context = (Context_a*)context_;
	MQTTAsync client = context->client;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer; // Response Options (... = [Default Initializer Macro])
	int rc; // >main

    printf("               [LOG] Agent: Successful connection\n");
    printf("               [LOG] Agent: Subscribing to topic %s for client %s using QoS-%d\n"
           "\n               [LOG] Will disconnect after all messages are received.\n\n", context->topic_a, context->username_a, QOS_S);

    // Connection Parameters
    opts.onSuccess = onSubscribe_a;
    opts.onFailure = onSubscribeFailure_a;
    opts.context = context;

    // Subscribe To Topic
    if ((rc = MQTTAsync_subscribe(client, context->topic_a, QOS_S, &opts)) != MQTTASYNC_SUCCESS)
    {
        printf("               [LOG] Agent: Failed to start subscribe, return code %d\n", rc);
        finished_sub = 1;
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
	finished_sub = 0;

	// Create Client

    if ((rc = MQTTAsync_create(&client, ADDRESS_S, username_a, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
        printf("               [LOG] Agent: Failed to create client, return code %d\n", rc);
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
        printf("               [LOG] Agent: Failed to set callbacks, return code %d\n", rc);
        free(context);
        MQTTAsync_destroy(&client);
        return EXIT_FAILURE;
    }

	// Set Connection Parameters

	conn_opts.keepAliveInterval = 30;
	conn_opts.cleansession = 1; // Persistance
	conn_opts.onSuccess = onConnect_a; // Calls onConnect If Connect Successfuly
	conn_opts.onFailure = onConnectFailure_a; // Calls onConnectFailure If Connection Fails
	conn_opts.context = context;

	// Connect To Broker

    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
        printf("               [LOG] Agent: Failed to start connect, return code %d\n", rc);
        free(context);
        MQTTAsync_destroy(&client);
        return EXIT_FAILURE;
    }

	// Wait For Subscription

    while (!is_subscribed && !finished_sub) {
        #if defined(_WIN32)
            Sleep(DELAY_100_MS_MS);
        #else
            usleep(DELAY_100_MS_US);
        #endif
    }

    if (finished_sub) {
        MQTTAsync_destroy(&client);
        free(context);
        return EXIT_FAILURE;
    }

    while (*online) {
        #if defined(_WIN32)
            Sleep(DELAY_100_MS_MS);
        #else
            usleep(DELAY_100_MS_US);
        #endif
    }

	// Disconnection Parameters

	disc_opts.onSuccess = onDisconnect_a;
	disc_opts.onFailure = onDisconnectFailure_a;

	// Disconnect To Broker

    if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS) {
        printf("               [LOG] Agent: Failed to start disconnect, return code %d\n", rc);
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