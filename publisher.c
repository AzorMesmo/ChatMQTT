// Imports

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTAsync.h"

#if !defined(_WIN32)
#include <unistd.h>
#else
#include <windows.h>
#endif

#if defined(_WRS_KERNEL)
#include <OsWrapper.h>
#endif

// Parameters

#define ADDRESS_P     "tcp://localhost:1883" // Default: "tcp://test.mosquitto.org:1883"
#define QOS_P         2
#define TIMEOUT_P     10000L // L = Long Int (To Avoid Compilation Error Across Platforms)

char username_p[64];
char topic_p[64];
char payload_p[1024];

// Context

typedef struct 
{
    MQTTAsync client;
    char username_p[64];
    char topic_p[64];
    char payload_p[1024];   

} Context;

// Flags

int finished_p = 0; // Program Finished

// Function Prototypes

void onConnect_p(void* context_, MQTTAsync_successData* response);
void onConnectFailure_p(void* context_, MQTTAsync_failureData* response);
void onDisconnect_p(void* context_, MQTTAsync_successData* response);
void onDisconnectFailure_p(void* context_, MQTTAsync_failureData* response);
void onSend_p(void* context_, MQTTAsync_successData* response);
void onSendFailure_p(void* context_, MQTTAsync_failureData* response);
void connlost_p(void *context_, char *cause);
int messageArrived_p(void* context_, char* topicName, int topicLen, MQTTAsync_message* m);

// Callbacks

void connlost_p(void *context_, char *cause) // Connection Lost
{
    Context* context = (Context*)context_;
	MQTTAsync client = context->client; // Cast context Back To The Original Type (void* -> MQTTAsync) To Be Able To Use
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	int rc; // >main

	printf("\nPublisher: Connection lost\n");
	if (cause)
		printf("     cause: %s\n", cause);
	printf("Publisher: Reconnecting\n");

	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
    conn_opts.onSuccess = onConnect_p; // Calls onConnect If Connect Successfuly
	conn_opts.onFailure = onConnectFailure_p; // Calls onConnectFailure If Connection Fails
    conn_opts.context = context;

	// Try To Connect Again
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Publisher: Failed to start connect, return code %d\n", rc);
 		finished_p = 1;
	}
}

void onDisconnectFailure_p(void* context_, MQTTAsync_failureData* response) // Fails To Disconnect
{
	printf("Publisher: Disconnect failed\n");
	finished_p = 1;
}

void onDisconnect_p(void* context_, MQTTAsync_successData* response) // Disconnected Successfuly
{
	printf("Publisher: Successful disconnection\n");
	finished_p = 1;
}

void onSendFailure_p(void* context_, MQTTAsync_failureData* response) // Fails To Publish Message
{
    Context* context = (Context*)context_;
	MQTTAsync client = context->client; // >connlost
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer; // Disconnection Options (... = [Default Initializer Macro])
	int rc; // >main

	printf("Publisher: Message send failed token %d error code %d\n", response->token, response->code);

	// Disconnection Parameters
	opts.onSuccess = onDisconnect_p;
	opts.onFailure = onDisconnectFailure_p;
	opts.context = context;

	// Disconnect To Broker
	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Publisher: Failed to start disconnect, return code %d\n", rc);
        free(context);
		exit(EXIT_FAILURE);
	}
}

void onSend_p(void* context_, MQTTAsync_successData* response) // Publish Message Successfuly
{
    Context* context = (Context*)context_;
	MQTTAsync client = context->client; // >connlost
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer; // Disconnection Options (... = [Default Initializer Macro])
	int rc; // >main

	printf("Message with token value %d delivery confirmed\n", response->token);

	// Disconnection Parameters
	opts.onSuccess = onDisconnect_p;
	opts.onFailure = onDisconnectFailure_p;
	opts.context = context;

	// Disconnect To Broker
	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Publisher: Failed to start disconnect, return code %d\n", rc);
        free(context);
		exit(EXIT_FAILURE);
	}
}


void onConnectFailure_p(void* context_, MQTTAsync_failureData* response) // Fails To Connect
{
	printf("Publisher: Connect failed, rc %d\n", response ? response->code : 0);
	finished_p = 1;
}


void onConnect_p(void* context_, MQTTAsync_successData* response) // Connected Successfuly
{
    Context* context = (Context*)context_;
	MQTTAsync client = context->client; // >connlost
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer; // Response Options (... = [Default Initializer Macro])
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer; // Message Object (... = [Default Initializer Macro])
	int rc; // >main

	printf("Publisher: Successful connection\n");

	// Response & Message Parameters
	opts.onSuccess = onSend_p;
	opts.onFailure = onSendFailure_p;
	opts.context = context;
	pubmsg.payload = context->payload_p; // Message
	pubmsg.payloadlen = (int)strlen(context->payload_p); // Message Size
	pubmsg.qos = QOS_P;
	pubmsg.retained = 0; // If Message Will Be Retained By The Broker

	// Send Message
	if ((rc = MQTTAsync_sendMessage(client, context->topic_p, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Publisher: Failed to start sendMessage, return code %d\n", rc);
        free(context);
		exit(EXIT_FAILURE);
	}
}

// Message Handling

int messageArrived_p(void* context_, char* topicName, int topicLen, MQTTAsync_message* m) // Message Arrived
{
	// Not Expecting Any Messages
	return 1;
}

// Publish Functions

int publishStatus(char username_p[64], char topic_p[64], char payload_p[1024]) // Publish User Status (Online / Offline)
{
    MQTTAsync_setTraceLevel(MQTTASYNC_TRACE_PROTOCOL);
	MQTTAsync client; // Client (Handler) | Connection To Broker
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	int rc; // Return Code For Function Calls
    finished_p = 0; // Reset finished_p

	// Create Client
	if ((rc = MQTTAsync_create(&client, ADDRESS_P, username_p, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Publisher: Failed to create client object, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

    // Create Context
    Context* context = malloc(sizeof(Context));
    context->client = client;
    strcpy(context->username_p, username_p);
    strcpy(context->topic_p, topic_p);
    strcpy(context->payload_p, payload_p);

	// Set Callbacks
	// Uses Second Argument To Store The Context (State) | In This Case, Only The Client Itself
	if ((rc = MQTTAsync_setCallbacks(client, context, connlost_p, messageArrived_p, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Publisher: Failed to set callback, return code %d\n", rc);
        free(context);
		exit(EXIT_FAILURE);
	}

	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1; // Persistance
	conn_opts.onSuccess = onConnect_p; // Calls onConnect If Connect Successfuly
	conn_opts.onFailure = onConnectFailure_p; // Calls onConnectFailure If Connection Fails
	conn_opts.context = context;

	// Connect To Broker
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Publisher: Failed to start connect, return code %d\n", rc);
        free(context);
		exit(EXIT_FAILURE);
	}

	// Wait For Completion
	printf("Publisher: Waiting for publication of '%s' on topic %s for client with ClientID: %s\n", payload_p, topic_p, username_p);
	while (!finished_p)
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif

    // Exit
	MQTTAsync_destroy(&client);
    free(context);
 	return rc;
}