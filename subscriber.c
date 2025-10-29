// Imports

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTAsync.h"
#include "messages.h"

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
#define TIMEOUT_S     10000L
#define SUBSCRIBER_STATUS_TIMEOUT 40

// Context

typedef struct 
{
    MQTTAsync client;
    char username_s[64];
    char topic_s[64];
    int connection_timeout; // Seconds (0 To No Timeout)
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
int subscriberStatus(const char* username_p, const char* topic_p, LinkedList* status_list);
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

	printf("\nSubscriber: Connection lost\n");
	if (cause)
		printf("     cause: %s\n", cause);
	printf("Subscriber: Reconnecting\n");

	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect_s;
	conn_opts.onFailure = onConnectFailure_s;
	conn_opts.context = context;

	// Try To Connect Again
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Subscriber: Failed to start connect, return code %d\n", rc);
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

    printf("\nSubscriber: Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %s\n\n", buf);

    // mensagem de grupo
    if (strcmp(topicName, "GROUPS") == 0) {
        updateGroup(buf);
    }
    else if (strstr(topicName, "_Control") != NULL) {
        printf("Mensagem de controle recebida no tópico %s:\n", topicName);
        printf("Conteúdo: %s\n", buf);

        if (strstr(buf, "JOIN_REQUEST") != NULL) {
            printf("Solicitação de entrada recebida: %s\n", buf);
            listInsertStatus(context->message_list, buf);
        }
    }

    listInsertStatus(context->message_list, buf); // Insert Status In The List

	// Memory Management
    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);

    return 1;
}

void onDisconnectFailure_s(void* context_, MQTTAsync_failureData* response) // Fails To Disconnect
{
    printf("Subscriber: Disconnect failed, rc %d\n", response->code);
    disc_finished = 1;
}

void onDisconnect_s(void* context_, MQTTAsync_successData* response) // Disconnected Successfuly
{
    printf("Subscriber: Successful disconnection\n");
    disc_finished = 1;
}

void onSubscribe_s(void* context_, MQTTAsync_successData* response) // Subscribed Successfuly
{
    printf("Subscriber: Subscribe succeeded\n");
    subscribed = 1;
}

void onSubscribeFailure_s(void* context_, MQTTAsync_failureData* response) // Fails To Subscribe
{
    printf("Subscriber: Subscribe failed, rc %d\n", response->code);
    finished_subscribe = 1;
}


void onConnectFailure_s(void* context_, MQTTAsync_failureData* response) // Fails To Connect
{
    printf("Subscriber: Connect failed, rc %d\n", response->code);
    finished_subscribe = 1;
}


void onConnect_s(void* context_, MQTTAsync_successData* response) // Connected Successfuly
{
	Context_s* context = (Context_s*)context_;
	MQTTAsync client = context->client; // >connectionLost_s
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer; // Response Options (... = [Default Initializer Macro])
	int rc; // >main

    if (!context->connection_timeout)
    {
        printf("Subscriber: Successful connection\n");
        printf("Subscriber: Subscribing to topic %s for client %s using QoS-%d\n"
           "Type '/quit' to quit\n", context->topic_s, context->username_s, QOS_S);
    } else {
        printf("Subscriber: Successful connection (Timeout In %d Seconds)\n", context->connection_timeout);
        printf("Subscriber: Subscribing to topic %s for client %s using QoS-%d\n"
           "\nWill disconnect after %d seconds if no messages are received.\n\n", context->topic_s, context->username_s, QOS_S, context->connection_timeout);
    }

    // Connection Parameters
    opts.onSuccess = onSubscribe_s;
    opts.onFailure = onSubscribeFailure_s;
    opts.context = context;

    // Subscribe To Topic
    if ((rc = MQTTAsync_subscribe(client, context->topic_s, QOS_S, &opts)) != MQTTASYNC_SUCCESS)
    {
        printf("Subscriber: Failed to start subscribe, return code %d\n", rc);
        finished_subscribe = 1;
    }

    // Subscribe to GROUPS topic
    if ((rc = MQTTAsync_subscribe(client, "GROUPS", QOS_S, &opts)) != MQTTASYNC_SUCCESS) {
        printf("Subscriber: Failed to subscribe to GROUPS, return code %d\n", rc);
        finished_subscribe = 1;
    }
}


int subscriberStatus(const char* username_p, const char* topic_p, LinkedList* status_list) // Subscribe To A Topic And Handle Messages
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
    if ((rc = MQTTAsync_create(&client, ADDRESS_S, username_p, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
        printf("Subscriber: Failed to create client, return code %d\n", rc);
        return EXIT_FAILURE;
    }

	// Create Context
    Context_s* context = malloc(sizeof(Context_s));
    if (!context) {
        MQTTAsync_destroy(&client);
        return EXIT_FAILURE;
    }
    context->client = client;
    strncpy(context->username_s, username_p, sizeof(context->username_s)-1);
    context->username_s[sizeof(context->username_s)-1] = '\0';
    strncpy(context->topic_s, topic_p, sizeof(context->topic_s)-1);
    context->topic_s[sizeof(context->topic_s)-1] = '\0';
    context->connection_timeout = SUBSCRIBER_STATUS_TIMEOUT;
    context->message_list = status_list;

    if ((rc = MQTTAsync_setCallbacks(client, context, connectionLost_s, messageArrived_s, NULL)) != MQTTASYNC_SUCCESS)
	{
        printf("Subscriber: Failed to set callbacks, return code %d\n", rc);
        free(context);
        MQTTAsync_destroy(&client);
        return EXIT_FAILURE;
    }

	// Connection Parameters
	conn_opts.keepAliveInterval = 30;
	conn_opts.cleansession = 1; // Persistance
	conn_opts.onSuccess = onConnect_s; // Calls onConnect If Connect Successfuly
	conn_opts.onFailure = onConnectFailure_s; // Calls onConnectFailure If Connection Fails
	conn_opts.context = context;

	// Connect To Broker
    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
        printf("Subscriber: Failed to start connect, return code %d\n", rc);
        free(context);
        MQTTAsync_destroy(&client);
        return EXIT_FAILURE;
    }

	// Wait For Subscription
    while (!subscribed && !finished_subscribe) {
        #if defined(_WIN32)
            Sleep(100);
        #else
            usleep(10000L);
        #endif
    }

    if (finished_subscribe) {
        MQTTAsync_destroy(&client);
        free(context);
        return EXIT_FAILURE;
    }

	#if defined(_WIN32)
		Sleep(1000 * SUBSCRIBER_STATUS_TIMEOUT); // Wait 30 Seconds
	#else
		usleep(1000000L * SUBSCRIBER_STATUS_TIMEOUT); // Wait 30 Seconds
	#endif

	// Do Nothing Until User Input "/quit" (Then Quit)
	// char input[100];
	// do 
	// {
	// 	if (fgets(input, sizeof(input), stdin) != NULL)
	// 	{
    //         input[strcspn(input, "\n")] = '\0'; // Remove Newline Character
    //     }
	// } while (strcmp(input, "/quit") != 0);

	// Disconnection Parameters
	disc_opts.onSuccess = onDisconnect_s;
	disc_opts.onFailure = onDisconnectFailure_s;

	// Disconnect To Broker
    if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS) {
        printf("Subscriber: Failed to start disconnect, return code %d\n", rc);
        MQTTAsync_destroy(&client);
        free(context);
        return EXIT_FAILURE;
    }

	// Wait Disconnection
 	while (!disc_finished)
 	{
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif
 	}

    MQTTAsync_destroy(&client);
    free(context);
    return rc;
}
