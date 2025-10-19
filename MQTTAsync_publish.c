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

#define ADDRESS_P     "tcp://test.mosquitto.org:1883"
#define CLIENTID_P    "ExampleClientPub"
#define TOPIC_P       "MQTT Examples"
#define PAYLOAD_P     "Hello World!"
#define QOS_P         1
#define TIMEOUT_P     10000L // L = Long Int (To Avoid Compilation Error Across Platforms)

// Flags

int finished_publish = 0; // Program Finished

// Callbacks

void connlost_publish(void *context, char *cause) // Connection Lost
{
	MQTTAsync client = (MQTTAsync)context; // Cast context Back To The Original Type (void* -> MQTTAsync) To Be Able To Use
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	int rc; // >main:158

	printf("\nPublisher: Connection lost\n");
	if (cause)
		printf("     cause: %s\n", cause);
	printf("Publisher: Reconnecting\n");

	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;

	// Try To Connect Again
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Publisher: Failed to start connect, return code %d\n", rc);
 		finished_publish = 1;
	}
}

void onDisconnectFailure_publish(void* context, MQTTAsync_failureData* response) // Fails To Disconnect
{
	printf("Publisher: Disconnect failed\n");
	finished_publish = 1;
}

void onDisconnect_publish(void* context, MQTTAsync_successData* response) // Disconnected Successfuly
{
	printf("Publisher: Successful disconnection\n");
	finished_publish = 1;
}

void onSendFailure_publish(void* context, MQTTAsync_failureData* response) // Fails To Publish Message
{
	MQTTAsync client = (MQTTAsync)context; // >connlost:35
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer; // Disconnection Options (... = [Default Initializer Macro])
	int rc; // >main:158

	printf("Publisher: Message send failed token %d error code %d\n", response->token, response->code);

	// Disconnection Parameters
	opts.onSuccess = onDisconnect_publish;
	opts.onFailure = onDisconnectFailure_publish;
	opts.context = client;

	// Disconnect To Broker
	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Publisher: Failed to start disconnect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
}

void onSend_publish(void* context, MQTTAsync_successData* response) // Publish Message Successfuly
{
	MQTTAsync client = (MQTTAsync)context; // >connlost:35
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer; // Disconnection Options (... = [Default Initializer Macro])
	int rc; // >main:158

	printf("Message with token value %d delivery confirmed\n", response->token);

	// Disconnection Parameters
	opts.onSuccess = onDisconnect_publish;
	opts.onFailure = onDisconnectFailure_publish;
	opts.context = client;

	// Disconnect To Broker
	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Publisher: Failed to start disconnect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
}


void onConnectFailure_publish(void* context, MQTTAsync_failureData* response) // Fails To Connect
{
	printf("Publisher: Connect failed, rc %d\n", response ? response->code : 0);
	finished_publish = 1;
}


void onConnect_publish(void* context, MQTTAsync_successData* response) // Connected Successfuly
{
	MQTTAsync client = (MQTTAsync)context; // >connlost:35
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer; // Response Options (... = [Default Initializer Macro])
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer; // Message Object (... = [Default Initializer Macro])
	int rc; // >main:158

	printf("Publisher: Successful connection\n");

	// Response & Message Parameters
	opts.onSuccess = onSend_publish;
	opts.onFailure = onSendFailure_publish;
	opts.context = client;
	pubmsg.payload = PAYLOAD_P; // Message
	pubmsg.payloadlen = (int)strlen(PAYLOAD_P); // Message Size
	pubmsg.qos = QOS_P;
	pubmsg.retained = 0; // If Message Will Be Retained By The Broker

	// Send Message
	if ((rc = MQTTAsync_sendMessage(client, TOPIC_P, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Publisher: Failed to start sendMessage, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
}

// Message Handling

int messageArrived_publish(void* context, char* topicName, int topicLen, MQTTAsync_message* m) // Message Arrived
{
	// Not Expecting Any Messages
	return 1;
}

// Main Function

int main_publish(int argc, char* argv[])
{
	sleep(2.5);

	MQTTAsync client; // Client (Handler) | Connection To Broker
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	int rc; // Return Code For Function Calls

	const char* uri = (argc > 1) ? argv[1] : ADDRESS_P; // Prioritize URI By Command-Line Argument (If Any)
	printf("Publisher: Using server at %s\n", uri);

	// Create Client
	if ((rc = MQTTAsync_create(&client, uri, CLIENTID_P, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Publisher: Failed to create client object, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	// Set Callbacks
	// Uses Second Argument To Store The Context (State) | In This Case, Only The Client Itself
	if ((rc = MQTTAsync_setCallbacks(client, client, connlost_publish, messageArrived_publish, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Publisher: Failed to set callback, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1; // Persistance
	conn_opts.onSuccess = onConnect_publish; // Calls onConnect If Connect Successfuly
	conn_opts.onFailure = onConnectFailure_publish; // Calls onConnectFailure If Connection Fails
	conn_opts.context = client;

	// Connect To Broker
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Publisher: Failed to start connect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	// Wait For Completion
	printf("Publisher: Waiting for publication of %s"
         "on topic %s for client with ClientID: %s\n",
         PAYLOAD_P, TOPIC_P, CLIENTID_P);
	while (!finished_publish)
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif

	MQTTAsync_destroy(&client);
 	return rc;
}
