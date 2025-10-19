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

#define ADDRESS     "tcp://test.mosquitto.org:1883"
#define CLIENTID    "ExampleClientPub"
#define TOPIC       "MQTT Examples"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L // L = Long Int (To Avoid Compilation Error Across Platforms)

// Flags

int disc_finished = 0; // Disconnection Finished
int subscribed = 0; // Subscription Successful
int finished = 0; // Program Finished

//
// Publish
//
// Publish
//
// Publish
//
// Publish
//
// Publish
//
// Publish
//
// Publish
//

// Callbacks

void connlost(void *context, char *cause) // Connection Lost
{
	MQTTAsync client = (MQTTAsync)context; // Cast context Back To The Original Type (void* -> MQTTAsync) To Be Able To Use
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	int rc; // >main:158

	printf("\nConnection lost\n");
	if (cause)
		printf("     cause: %s\n", cause);
	printf("Reconnecting\n");

	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;

	// Try To Connect Again
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
 		finished = 1;
	}
}

void onDisconnectFailure(void* context, MQTTAsync_failureData* response) // Fails To Disconnect
{
	printf("Disconnect failed\n");
	finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response) // Disconnected Successfuly
{
	printf("Successful disconnection\n");
	finished = 1;
}

void onSendFailure(void* context, MQTTAsync_failureData* response) // Fails To Publish Message
{
	MQTTAsync client = (MQTTAsync)context; // >connlost:35
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer; // Disconnection Options (... = [Default Initializer Macro])
	int rc; // >main:158

	printf("Message send failed token %d error code %d\n", response->token, response->code);

	// Disconnection Parameters
	opts.onSuccess = onDisconnect;
	opts.onFailure = onDisconnectFailure;
	opts.context = client;

	// Disconnect To Broker
	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
}

void onSend(void* context, MQTTAsync_successData* response) // Publish Message Successfuly
{
	MQTTAsync client = (MQTTAsync)context; // >connlost:35
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer; // Disconnection Options (... = [Default Initializer Macro])
	int rc; // >main:158

	printf("Message with token value %d delivery confirmed\n", response->token);

	// Disconnection Parameters
	opts.onSuccess = onDisconnect;
	opts.onFailure = onDisconnectFailure;
	opts.context = client;

	// Disconnect To Broker
	if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
}


void onConnectFailure(void* context, MQTTAsync_failureData* response) // Fails To Connect
{
	printf("Connect failed, rc %d\n", response ? response->code : 0);
	finished = 1;
}


void onConnect(void* context, MQTTAsync_successData* response) // Connected Successfuly
{
	MQTTAsync client = (MQTTAsync)context; // >connlost:35
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer; // Response Options (... = [Default Initializer Macro])
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer; // Message Object (... = [Default Initializer Macro])
	int rc; // >main:158

	printf("Successful connection\n");

	// Response & Message Parameters
	opts.onSuccess = onSend;
	opts.onFailure = onSendFailure;
	opts.context = client;
	pubmsg.payload = PAYLOAD; // Message
	pubmsg.payloadlen = (int)strlen(PAYLOAD); // Message Size
	pubmsg.qos = QOS;
	pubmsg.retained = 0; // If Message Will Be Retained By The Broker

	// Send Message
	if ((rc = MQTTAsync_sendMessage(client, TOPIC, &pubmsg, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start sendMessage, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}
}

// Message Handling

int messageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* m) // Message Arrived
{
	// Not Expecting Any Messages
	return 1;
}

// Main Function

int main(int argc, char* argv[])
{
	MQTTAsync client; // Client (Handler) | Connection To Broker
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	int rc; // Return Code For Function Calls

	const char* uri = (argc > 1) ? argv[1] : ADDRESS; // Prioritize URI By Command-Line Argument (If Any)
	printf("Using server at %s\n", uri);

	// Create Client
	if ((rc = MQTTAsync_create(&client, uri, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to create client object, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	// Set Callbacks
	// Uses Second Argument To Store The Context (State) | In This Case, Only The Client Itself
	if ((rc = MQTTAsync_setCallbacks(client, client, connlost, messageArrived, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to set callback, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1; // Persistance
	conn_opts.onSuccess = onConnect; // Calls onConnect If Connect Successfuly
	conn_opts.onFailure = onConnectFailure; // Calls onConnectFailure If Connection Fails
	conn_opts.context = client;

	// Connect To Broker
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		exit(EXIT_FAILURE);
	}

	// Wait For Completion
	printf("Waiting for publication of %s\n"
         "on topic %s for client with ClientID: %s\n",
         PAYLOAD, TOPIC, CLIENTID);
	while (!finished)
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif

	MQTTAsync_destroy(&client);
 	return rc;
}

//
// Subscribe
//
// Subscribe
//
// Subscribe
//
// Subscribe
//
// Subscribe
//
// Subscribe
//
// Subscribe
//

// Callbacks

void onConnect(void* context, MQTTAsync_successData* response); // Connected Successfuly
void onConnectFailure(void* context, MQTTAsync_failureData* response); // Fails To Connect

void connlost(void *context, char *cause) // Connection Lost
{
	MQTTAsync client = (MQTTAsync)context; // Cast context Back To The Original Type (void* -> MQTTAsync) To Be Able To Use
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	int rc; // >main:

	printf("\nConnection lost\n");
	if (cause)
		printf("     cause: %s\n", cause);
	printf("Reconnecting\n");

	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;

	// Try To Connect Again
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		finished = 1;
	}
}


int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message) // Message Received
{
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n", message->payloadlen, (char*)message->payload); // .* = Print This Exactly Lenght
    
	// Memory Management
	MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);

    return 1;
}

void onDisconnectFailure(void* context, MQTTAsync_failureData* response) // Fails To Disconnect
{
	printf("Disconnect failed, rc %d\n", response->code);
	disc_finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response) // Disconnected Successfuly
{
	printf("Successful disconnection\n");
	disc_finished = 1;
}

void onSubscribe(void* context, MQTTAsync_successData* response) // Subscribed Successfuly
{
	printf("Subscribe succeeded\n");
	subscribed = 1;
}

void onSubscribeFailure(void* context, MQTTAsync_failureData* response) // Fails To Subscribe
{
	printf("Subscribe failed, rc %d\n", response->code);
	finished = 1;
}


void onConnectFailure(void* context, MQTTAsync_failureData* response) // Fails To Connect
{
	printf("Connect failed, rc %d\n", response->code);
	finished = 1;
}


void onConnect(void* context, MQTTAsync_successData* response) // Connected Successfuly
{
	MQTTAsync client = (MQTTAsync)context; // >connlost:40
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer; // Response Options (... = [Default Initializer Macro])
	int rc; // >main:

	printf("Successful connection\n");
	printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);

	// Connection Parameters
	opts.onSuccess = onSubscribe;
	opts.onFailure = onSubscribeFailure;
	opts.context = client;

	// Subscribe To Topic
	if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start subscribe, return code %d\n", rc);
		finished = 1;
	}
}


int main(int argc, char* argv[])
{
	MQTTAsync client; // Client (Handler) | Connection To Broker
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer; // Disconnection Options (... = [Default Initializer Macro])
	int rc; // Return Code For Function Calls
	int ch; // Input Character Storage | int <-> getchar()

	const char* uri = (argc > 1) ? argv[1] : ADDRESS; // Prioritize URI By Command-Line Argument (If Any)
	printf("Using server at %s\n", uri);

	// Create Client
	if ((rc = MQTTAsync_create(&client, uri, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL))
			!= MQTTASYNC_SUCCESS)
	{
		printf("Failed to create client, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto exit;
	}

	// Set Callbacks
	// Uses Second Argument To Store The Context (State) | In This Case, Only The Client Itself
	if ((rc = MQTTAsync_setCallbacks(client, client, connlost, msgarrvd, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to set callbacks, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}

	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1; // Persistance
	conn_opts.onSuccess = onConnect; // Calls onConnect If Connect Successfuly
	conn_opts.onFailure = onConnectFailure; // Calls onConnectFailure If Connection Fails
	conn_opts.context = client;

	// Connect To Broker
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}

	// Wait Subscription Or Errors
	while (!subscribed && !finished)
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif

	if (finished)
		goto exit;

	// Do Nothing Until User Input "Q"/"q" (Then Quit)
	do 
	{
		ch = getchar();
	} while (ch!='Q' && ch != 'q');

	// Disconnection Parameters
	disc_opts.onSuccess = onDisconnect;
	disc_opts.onFailure = onDisconnectFailure;

	// Disconnect To Broker
	if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start disconnect, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
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

destroy_exit:
	MQTTAsync_destroy(&client);
exit:
 	return rc;
}

//
// Test
//
// Test
//
// Test
//
// Test
//
// Test
//
// Test
//
// Test
//

// PUBLISH
//
// connlost
// onDisconnectFailure
// onDisconnect
// onSendFailure
// onSend
// onConnectFailure
// onConnect
//
// messageArrived
//
// main

// SUBSCRIBE
//
// onConnect*
// onConnectFailure*
//
// connlost
// msgarrvd
// onDisconnectFailure
// onDisconnect
// onSubscribeFailure
// onSubscribe
// onConnectFailure
// onConnect
//
// main

// TEST
//
// onConnectFailure*
// onConnect*
//
// connlost            [OK]
// msgarrvd            [OK]
// onDisconnectFailure [OK]
// onDisconnect        [OK]
// onSubscribeFailure
// onSubscribe
// onSendFailure
// onSend
// onConnectFailure
// onConnect
//
// main

// Callbacks

void connlost(void *context, char *cause) // Connection Lost
{
	MQTTAsync client = (MQTTAsync)context; // Cast context Back To The Original Type (void* -> MQTTAsync) To Be Able To Use
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	int rc; // Return Code

	printf("\nConnection lost\n");
	if (cause)
		printf("     cause: %s\n", cause);
	printf("Reconnecting\n");

	// Connection Parameters
	conn_opts.keepAliveInterval = 30;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect;
	conn_opts.onFailure = onConnectFailure;

	// Try To Connect Again
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		finished = 1;
	}
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message) // Message Received
{
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n", message->payloadlen, (char*)message->payload); // .* = Print This Exactly Lenght
    
	// Memory Management
	MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);

    return 1;
}

void onDisconnectFailure(void* context, MQTTAsync_failureData* response) // Fails To Disconnect
{
    printf("\nDisconnect failed\n");
	if (response->code)
		printf("     code: %s\n", response->code);
	disc_finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response) // Disconnected Successfuly
{
	printf("Successful disconnection\n");
	disc_finished = 1;
}