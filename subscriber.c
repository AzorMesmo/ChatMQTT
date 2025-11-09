// Imports

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTAsync.h"
#include "constants.h"
#include "messages.h"

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
    char username_s[64];
    char topic_s[64];
    int no_timeout;
    LinkedList* message_list;
} Context_s;

// Flags

int disc_finished = 0; // Disconnection Finished
int subscribed = 0; // Subscription Successful
int finished_subscribe = 0; // Program Finished

// Function Prototypes

void onConnect_s(void* context_, MQTTAsync_successData* response);
void onConnectFailure_s(void* context_, MQTTAsync_failureData* response);
void connectionLost_s(void *context_, char *cause);
int messageArrived_s(void *context_, char *topicName, int topicLen, MQTTAsync_message *message);
void onDisconnectFailure_s(void* context_, MQTTAsync_failureData* response);
void onDisconnect_s(void* context_, MQTTAsync_successData* response);
void onSubscribe_s(void* context_, MQTTAsync_successData* response);
void onSubscribeFailure_s(void* context_, MQTTAsync_failureData* response);
int subscriberRetained(const char* username_s, const char* topic_s, LinkedList* status_list);
void updateGroup(const char* payload);
void processGroupRequest(const char* payload, const char* username);
void processGroupResponse(const char* payload, const char* username);



// Callbacks

void connectionLost_s(void *context_, char *cause) // Connection Lost
{
	Context_s* context = (Context_s*)context_;
	MQTTAsync client = context->client; // Cast context Back To The Original Type (void* -> MQTTAsync) To Be Able To Use
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	int rc; // >main

    if (LOG_ENABLED)
    {
        printf("\n               [LOG] SUBSCRIBER: Connection lost\n");
        if (cause)
            printf("     cause: %s\n", cause);
        printf("               [LOG] SUBSCRIBER: Reconnecting\n");
    }

	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect_s;
	conn_opts.onFailure = onConnectFailure_s;
	conn_opts.context = context;

	// Try To Connect Again
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("SUBSCRIBER: Failed to start connect, return code %d\n", rc);
		finished_subscribe = 1;
	}
}

int messageArrived_s(void *context_, char *topicName, int topicLen, MQTTAsync_message *message) // Message Arrived
{
    Context_s* context = (Context_s*)context_;

    // Ensure Payload Is Nul-Terminated

    char buf[1024];
    int len = message->payloadlen;
    if (len >= (int)sizeof(buf)) len = sizeof(buf) - 1;
    memcpy(buf, message->payload, len);
    buf[len] = '\0';

    if (LOG_ENABLED)
    {
        printf("\n               [LOG] SUBSCRIBER: Message arrived\n");
        printf("               [LOG]      Topic: %s\n", topicName);
        printf("               [LOG]    Message: %s\n", buf);
    }

    // Message Type

    if (strstr(topicName, "USERS") != NULL) // Users
    { 
        if (LOG_ENABLED)
            printf("               [LOG] SUBSCRIBER: Users status update received.\n");
        listInsert(context->message_list, buf);
    }
    else if (strstr(topicName, "GROUPS") != NULL) // Groups 
    {
        if (LOG_ENABLED)
            printf("               [LOG] SUBSCRIBER: Groups update received.\n");
        listInsert(context->message_list, buf);
    }
    else if (strstr(topicName, "USER_REQUEST") != NULL) // User Requests
    {
        if (LOG_ENABLED)
            printf("               [LOG] SUBSCRIBER: Requests update received.\n");
        listInsert(context->message_list, buf);
    }
    else if (strstr(topicName, "GROUP_REQUEST") != NULL) // User Requests
    {
        if (LOG_ENABLED)
            printf("               [LOG] SUBSCRIBER: Requests update received.\n");
        listInsert(context->message_list, buf);
    }

	// Memory Management
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);

    return 1;
}

void onDisconnectFailure_s(void* context_, MQTTAsync_failureData* response) // Fails To Disconnect
{
    if (LOG_ENABLED)
        printf("               [LOG] SUBSCRIBER: Disconnect failed, rc %d\n", response->code);
    disc_finished = 1;
}

void onDisconnect_s(void* context_, MQTTAsync_successData* response) // Disconnected Successfuly
{
    if (LOG_ENABLED)
        printf("               [LOG] SUBSCRIBER: Successful disconnection\n");
    disc_finished = 1;
}

void onSubscribe_s(void* context_, MQTTAsync_successData* response) // Subscribed Successfuly
{
    if (LOG_ENABLED)
        printf("               [LOG] SUBSCRIBER: Subscribe succeeded\n");
    subscribed = 1;
}

void onSubscribeFailure_s(void* context_, MQTTAsync_failureData* response) // Fails To Subscribe
{
    if (LOG_ENABLED)
        printf("               [LOG] SUBSCRIBER: Subscribe failed, rc %d\n", response->code);
    finished_subscribe = 1;
}


void onConnectFailure_s(void* context_, MQTTAsync_failureData* response) // Fails To Connect
{
    if (LOG_ENABLED)
        printf("               [LOG] SUBSCRIBER: Connect failed, rc %d\n", response->code);
    finished_subscribe = 1;
}


void onConnect_s(void* context_, MQTTAsync_successData* response) // Connected Successfuly
{
	Context_s* context = (Context_s*)context_;
	MQTTAsync client = context->client; // >connectionLost_s
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer; // Response Options (... = [Default Initializer Macro])
	int rc; // >main

    if (LOG_ENABLED)
    {
        printf("               [LOG] SUBSCRIBER: Successful connection\n");
        if (context->no_timeout)
        {
            printf("               [LOG] SUBSCRIBER: Subscribing to topic %s for client %s using QoS-%d\n"
                   "               [LOG] SUBSCRIBER: Type '/quit' to quit\n", context->topic_s, context->username_s, QOS);
        } else {
            printf("               [LOG] SUBSCRIBER: Subscribing to topic %s for client %s using QoS-%d\n"
                   "               [LOG] SUBSCRIBER: Will disconnect after all messages are received.\n", context->topic_s, context->username_s, QOS);
        }
    }

    // Connection Parameters
    opts.onSuccess = onSubscribe_s;
    opts.onFailure = onSubscribeFailure_s;
    opts.context = context;

    // Subscribe To Topic
    if ((rc = MQTTAsync_subscribe(client, context->topic_s, QOS, &opts)) != MQTTASYNC_SUCCESS)
    {
        if (LOG_ENABLED)
            printf("               [LOG] SUBSCRIBER: Failed to start subscribe, return code %d\n", rc);
        finished_subscribe = 1;
    }

    // Subscribe to GROUPS topic
    // if ((rc = MQTTAsync_subscribe(client, "GROUPS", QOS, &opts)) != MQTTASYNC_SUCCESS) {
    //     printf("               [LOG] SUBSCRIBER: Failed to subscribe to GROUPS, return code %d\n", rc);
    //     finished_subscribe = 1;
    // }
}

// Main Functions

int subscriberRetained(const char* username_s, const char* topic_s, LinkedList* status_list) // Subscribe To A Retained Topic (Not Mantain Connection)
{
	MQTTAsync client; // Client (Handler) | Connection To Broker
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer; // Disconnection Options (... = [Default Initializer Macro])
	int rc; // Return Code For Function Calls
	int ch; // Input Character Storage | int <-> getchar()

	// Reset Parameters

	disc_finished = 0;
	subscribed = 0;
	finished_subscribe = 0;

	// Create Client

    if ((rc = MQTTAsync_create(&client, ADDRESS, username_s, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
        if (LOG_ENABLED)
            printf("               [LOG] SUBSCRIBER: Failed to create client, return code %d\n", rc);
        return EXIT_FAILURE;
    }

	// Create Context

    Context_s* context = malloc(sizeof(Context_s));
    if (!context) {
        MQTTAsync_destroy(&client);
        return EXIT_FAILURE;
    }

    context->client = client; // Client
    strcpy(context->username_s, username_s); // Username
    strcpy(context->topic_s, topic_s); // Topic
    context->message_list = status_list; // Status (Message) List

    // Set Callbacks

    if ((rc = MQTTAsync_setCallbacks(client, context, connectionLost_s, messageArrived_s, NULL)) != MQTTASYNC_SUCCESS)
	{
        if (LOG_ENABLED)
            printf("               [LOG] SUBSCRIBER: Failed to set callbacks, return code %d\n", rc);
        free(context);
        MQTTAsync_destroy(&client);
        return EXIT_FAILURE;
    }

	// Set Connection Parameters

	conn_opts.keepAliveInterval = 30;
	conn_opts.cleansession = 1; // Persistance
	conn_opts.onSuccess = onConnect_s; // Calls onConnect If Connect Successfuly
	conn_opts.onFailure = onConnectFailure_s; // Calls onConnectFailure If Connection Fails
	conn_opts.context = context;

	// Connect To Broker

    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
        if (LOG_ENABLED)
            printf("               [LOG] SUBSCRIBER: Failed to start connect, return code %d\n", rc);
        free(context);
        MQTTAsync_destroy(&client);
        return EXIT_FAILURE;
    }

	// Wait For Subscription

    while (!subscribed && !finished_subscribe) {
        #if defined(_WIN32)
            Sleep(DELAY_100_MS_MS);
        #else
            usleep(DELAY_100_MS_US);
        #endif
    }

    if (finished_subscribe) {
        MQTTAsync_destroy(&client);
        free(context);
        return EXIT_FAILURE;
    }

	// Do Nothing Until User Input "/quit" (Then Quit)
	// char input[100];
	// do 
	// {
	// 	if (fgets(input, sizeof(input), stdin) != NULL)
	// 	{
    //         input[strcspn(input, "\n")] = '\0'; // Remove Newline Character
    //     }
	// } while (strcmp(input, "/quit") != 0);

    // Wait Until All Messages Are Received

    int waited_ms = 0;
    const int step_ms = 100; // 0.1 Second
    const int max_ms = 10000; // 10 Seconds
    while (waited_ms < max_ms) {
        pthread_mutex_lock(&context->message_list->lock);
        int has_msg = (context->message_list->head != NULL);
        pthread_mutex_unlock(&context->message_list->lock);
        if (has_msg) break;
        #if defined(_WIN32)
            Sleep(step_ms);
        #else
            usleep(step_ms * 1000);
        #endif
        waited_ms += step_ms;
    }

	// Disconnection Parameters

	disc_opts.onSuccess = onDisconnect_s;
	disc_opts.onFailure = onDisconnectFailure_s;

	// Disconnect To Broker

    if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS) {
        if (LOG_ENABLED)
            printf("               [LOG] SUBSCRIBER: Failed to start disconnect, return code %d\n", rc);
        MQTTAsync_destroy(&client);
        free(context);
        return EXIT_FAILURE;
    }

	// Wait Disconnection

 	while (!disc_finished)
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
