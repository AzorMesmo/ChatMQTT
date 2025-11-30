// Clear Topic Mannualy: mosquitto_pub -t "[TOPIC]" -r -n

// Imports

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTAsync.h"
#include "constants.h"

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
    char username_p[64];
    char topic_p[1024];
    char payload_p[1024];
	int retained;
} Context_p;

// Flags

int finished_p = 0; // Program Finished

// Function Prototypes

void onConnect_p(void* context_, MQTTAsync_successData* response);
void onConnectFailure_p(void* context_, MQTTAsync_failureData* response);
void onDisconnect_p(void* context_, MQTTAsync_successData* response);
void onDisconnectFailure_p(void* context_, MQTTAsync_failureData* response);
void onSend_p(void* context_, MQTTAsync_successData* response);
void onSendFailure_p(void* context_, MQTTAsync_failureData* response);
void connectionLost_p(void *context_, char *cause);
int messageArrived_p(void* context_, char* topicName, int topicLen, MQTTAsync_message* m);
int publisher(const char* username_p, const char* topic_p, const char* payload_p, int retained);

// Callbacks

void connectionLost_p(void *context_, char *cause) // Connection Lost
{
    Context_p* context = (Context_p*)context_;
	MQTTAsync client = context->client; // Cast context Back To The Original Type (void* -> MQTTAsync) To Be Able To Use
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	int rc;

	if (LOG_ENABLED)
	{
		printf("\n               [LOG] PUBLISHER: Connection lost\n");
		if (cause)
			printf("     cause: %s\n", cause);
		printf("               [LOG] PUBLISHER: Reconnecting\n");
	}
	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
    conn_opts.onSuccess = onConnect_p;
	conn_opts.onFailure = onConnectFailure_p;
    conn_opts.context = context;

	// Try To Connect Again
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		if (LOG_ENABLED)
			printf("               [LOG] PUBLISHER: Failed to start connect, return code %d\n", rc);
 		finished_p = 1;
	}
}

void onDisconnectFailure_p(void* context_, MQTTAsync_failureData* response) // Fails To Disconnect
{
	if (LOG_ENABLED)
		printf("               [LOG] PUBLISHER: Disconnect failed\n");
	finished_p = 1;
}

void onDisconnect_p(void* context_, MQTTAsync_successData* response) // Disconnected Successfuly
{
	if (LOG_ENABLED)
		printf("               [LOG] PUBLISHER: Successful disconnection\n");
	finished_p = 1;
}

void onSendFailure_p(void* context_, MQTTAsync_failureData* response) // Fails To Publish Message
{
    Context_p* context = (Context_p*)context_;
	MQTTAsync client = context->client;
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer; // Disconnection Options (... = [Default Initializer Macro])
	int rc;

	if (LOG_ENABLED)
		printf("               [LOG] PUBLISHER: Message send failed token %d error code %d\n", response->token, response->code);

	// Disconnection Parameters
	opts.onSuccess = onDisconnect_p;
	opts.onFailure = onDisconnectFailure_p;
	opts.context = context;

	// Disconnect To Broker
	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	{
		if (LOG_ENABLED)
			printf("               [LOG] PUBLISHER: Failed to start disconnect, return code %d\n", rc);
        free(context);
		exit(EXIT_FAILURE);
	}
}

void onSend_p(void* context_, MQTTAsync_successData* response) // Publish Message Successfuly
{
    Context_p* context = (Context_p*)context_;
	MQTTAsync client = context->client;
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer; // Disconnection Options (... = [Default Initializer Macro])
	int rc;

	if (LOG_ENABLED)
		printf("               [LOG] PUBLISHER: Message with token value %d delivery confirmed\n", response->token);

	// Disconnection Parameters
	opts.onSuccess = onDisconnect_p;
	opts.onFailure = onDisconnectFailure_p;
	opts.context = context;

	// Disconnect To Broker
	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	{
		if (LOG_ENABLED)
			printf("               [LOG] PUBLISHER: Failed to start disconnect, return code %d\n", rc);
        free(context);
		exit(EXIT_FAILURE);
	}
}


void onConnectFailure_p(void* context_, MQTTAsync_failureData* response) // Fails To Connect
{
	if (LOG_ENABLED)
		printf("               [LOG] PUBLISHER: Connect failed, rc %d\n", response ? response->code : 0);
	finished_p = 1;
}


void onConnect_p(void* context_, MQTTAsync_successData* response) // Connected Successfuly
{
    Context_p* context = (Context_p*)context_;
	MQTTAsync client = context->client; // >connlost
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer; // Response Options (... = [Default Initializer Macro])
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer; // Message Object (... = [Default Initializer Macro])
	int rc;

	if (LOG_ENABLED)
		printf("               [LOG] PUBLISHER: Successful connection\n");

	// Response & Message Parameters
	opts.onSuccess = onSend_p;
	opts.onFailure = onSendFailure_p;
	opts.context = context;
	pubmsg.payload = context->payload_p; // Message
	pubmsg.payloadlen = (int)strlen(context->payload_p); // Message Size
	pubmsg.qos = QOS;
	pubmsg.retained = context->retained; // If Message Will Be Retained By The Broker

	// Send Message
	if ((rc = MQTTAsync_sendMessage(client, context->topic_p, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	{
		if (LOG_ENABLED)
			printf("               [LOG] PUBLISHER: Failed to start sendMessage, return code %d\n", rc);
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

// Main Functions

int publisher(const char* username_p, const char* topic_p, const char* payload_p, int retained) // Publish Messages
{
	MQTTAsync client; // Client (Handler) | Connection To Broker
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	int rc; // Return Code For Function Calls
    // Reset Parameters
	finished_p = 0;

	// Create Client

	if ((rc = MQTTAsync_create(&client, ADDRESS, username_p, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
		if (LOG_ENABLED)
			printf("               [LOG] PUBLISHER: Failed to create client object, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

    // Create Context

    Context_p* context = malloc(sizeof(Context_p));
    context->client = client;
    strcpy(context->username_p, username_p);
    strcpy(context->topic_p, topic_p);
    strcpy(context->payload_p, payload_p);
	context->retained = retained;

	// Set Callbacks

	if ((rc = MQTTAsync_setCallbacks(client, context, connectionLost_p, messageArrived_p, NULL)) != MQTTASYNC_SUCCESS)
	{
		if (LOG_ENABLED)
			printf("               [LOG] PUBLISHER: Failed to set callback, return code %d\n", rc);
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
		if (LOG_ENABLED)
			printf("               [LOG] PUBLISHER: Failed to start connect, return code %d\n", rc);
        free(context);
		exit(EXIT_FAILURE);
	}

	// Wait For Completion

	if (LOG_ENABLED)
		printf("               [LOG] PUBLISHER: Waiting for publication of '%s' on topic %s for client with ClientID: %s\n", payload_p, topic_p, username_p);
	while (!finished_p)
		#if defined(_WIN32)
            Sleep(DELAY_100_MS_MS);
        #else
            usleep(DELAY_100_MS_US);
        #endif

    // Exit

	MQTTAsync_destroy(&client);
    free(context);
 	return rc;
}

int publisherDirty(const char* username_p, const char* topic_p, const char* payload_p, int retained) // Publish Messages
{
	MQTTAsync client; // Client (Handler) | Connection To Broker
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	int rc; // Return Code For Function Calls
    // Reset Parameters
	finished_p = 0;

	// Create Client

	if ((rc = MQTTAsync_create(&client, ADDRESS, username_p, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
		if (LOG_ENABLED)
			printf("               [LOG] PUBLISHER: Failed to create client object, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

    // Create Context

    Context_p* context = malloc(sizeof(Context_p));
    context->client = client;
    strcpy(context->username_p, username_p);
    strcpy(context->topic_p, topic_p);
    strcpy(context->payload_p, payload_p);
	context->retained = retained;

	// Set Callbacks

	if ((rc = MQTTAsync_setCallbacks(client, context, connectionLost_p, messageArrived_p, NULL)) != MQTTASYNC_SUCCESS)
	{
		if (LOG_ENABLED)
			printf("               [LOG] PUBLISHER: Failed to set callback, return code %d\n", rc);
        free(context);
		exit(EXIT_FAILURE);
	}

	// Connection Parameters

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 0; // Persistance
	conn_opts.onSuccess = onConnect_p; // Calls onConnect If Connect Successfuly
	conn_opts.onFailure = onConnectFailure_p; // Calls onConnectFailure If Connection Fails
	conn_opts.context = context;

	// Connect To Broker

	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		if (LOG_ENABLED)
			printf("               [LOG] PUBLISHER: Failed to start connect, return code %d\n", rc);
        free(context);
		exit(EXIT_FAILURE);
	}

	// Wait For Completion

	if (LOG_ENABLED)
		printf("               [LOG] PUBLISHER: Waiting for publication of '%s' on topic %s for client with ClientID: %s\n", payload_p, topic_p, username_p);
	while (!finished_p)
		#if defined(_WIN32)
            Sleep(DELAY_100_MS_MS);
        #else
            usleep(DELAY_100_MS_US);
        #endif

    // Exit

	MQTTAsync_destroy(&client);
    free(context);
 	return rc;
}