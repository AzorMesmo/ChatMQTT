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

#define ADDRESS_S     "tcp://localhost:1883" // Default: "tcp://test.mosquitto.org:1883"
#define CLIENTID_S    "ExampleClientSub"
#define TOPIC_S       "MQTT Examples"
#define PAYLOAD_S     "Hello World!"
#define QOS_S         1
#define TIMEOUT_S     10000L // L = Long Int (To Avoid Compilation Error Across Platforms)

// Flags

int disc_finished = 0; // Disconnection Finished
int subscribed = 0; // Subscription Successful
int finished_subscribe = 0; // Program Finished

// Callbacks

void onConnect_subscribe(void* context, MQTTAsync_successData* response); // Connected Successfuly
void onConnectFailure_subscribe(void* context, MQTTAsync_failureData* response); // Fails To Connect
void updateGroup(const char* payload);


void connlost(void *context, char *cause) // Connection Lost
{
	MQTTAsync client = (MQTTAsync)context; // Cast context Back To The Original Type (void* -> MQTTAsync) To Be Able To Use
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	int rc; // >main:138

	printf("\nSubscriber: Connection lost\n");
	if (cause)
		printf("     cause: %s\n", cause);
	printf("Subscriber: Reconnecting\n");

	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.onSuccess = onConnect_subscribe;
	conn_opts.onFailure = onConnectFailure_subscribe;

	// Try To Connect Again
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Subscriber: Failed to start connect, return code %d\n", rc);
		finished_subscribe = 1;
	}
}


int msgarrvd_subscribe(void *context, char *topicName, int topicLen, MQTTAsync_message *message) // Message Received
{
	char payload[1024];
    strncpy(payload, (char*)message->payload, message->payloadlen);
    payload[message->payloadlen] = '\0';

    // Se a mensagem for do tópico GROUPS, atualiza os grupos
    if (strcmp(topicName, "GROUPS") == 0) {
        updateGroup(payload);
    }


    printf("Subscriber: Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n", message->payloadlen, (char*)message->payload); // .* = Print This Exactly Lenght
    
	// Memory Management
	MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName);

    return 1;
}

void onDisconnectFailure_subscribe(void* context, MQTTAsync_failureData* response) // Fails To Disconnect
{
	printf("Subscriber: Disconnect failed, rc %d\n", response->code);
	disc_finished = 1;
}

void onDisconnect_subscribe(void* context, MQTTAsync_successData* response) // Disconnected Successfuly
{
	printf("Subscriber: Successful disconnection\n");
	disc_finished = 1;
}

void onSubscribe_subscribe(void* context, MQTTAsync_successData* response) // Subscribed Successfuly
{
	printf("Subscriber: Subscribe succeeded\n");
	subscribed = 1;
}

void onSubscribeFailure_subscribe(void* context, MQTTAsync_failureData* response) // Fails To Subscribe
{
	printf("Subscriber: Subscribe failed, rc %d\n", response->code);
	finished_subscribe = 1;
}


void onConnectFailure_subscribe(void* context, MQTTAsync_failureData* response) // Fails To Connect
{
	printf("Subscriber: Connect failed, rc %d\n", response->code);
	finished_subscribe = 1;
}


void onConnect_subscribe(void* context, MQTTAsync_successData* response) // Connected Successfuly
{
	MQTTAsync client = (MQTTAsync)context; // >connlost:40
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer; // Response Options (... = [Default Initializer Macro])
	int rc; // >main:138

	printf("Subscriber: Successful connection\n");
	printf("Subscriber: Subscribing to topic %s for client %s using QoS-%d\n"
           "Press Q<Enter> to quit\n", TOPIC_S, CLIENTID_S, QOS_S);

	// Connection Parameters
	opts.onSuccess = onSubscribe_subscribe;
	opts.onFailure = onSubscribeFailure_subscribe;
	opts.context = client;

	// Subscribe To Topic
	if ((rc = MQTTAsync_subscribe(client, TOPIC_S, QOS_S, &opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Subscriber: Failed to start subscribe, return code %d\n", rc);
		finished_subscribe = 1;
	}
	// Subscribe To GROUPS Topic
	if ((rc = MQTTAsync_subscribe(client, "GROUPS", QOS_S, &opts)) != MQTTASYNC_SUCCESS)
    {
        printf("Subscriber: Failed to subscribe to GROUPS, return code %d\n", rc);
        finished_subscribe = 1;
    }
}


int main_subscribe(int argc, char* argv[])
{
	MQTTAsync client; // Client (Handler) | Connection To Broker
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer; // Connection Options (... = [Default Initializer Macro])
	MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer; // Disconnection Options (... = [Default Initializer Macro])
	int rc; // Return Code For Function Calls
	int ch; // Input Character Storage | int <-> getchar()

	const char* uri = (argc > 1) ? argv[1] : ADDRESS_S; // Prioritize URI By Command-Line Argument (If Any)
	printf("Subscriber: Using server at %s\n", uri);

	// Create Client
	if ((rc = MQTTAsync_create(&client, uri, CLIENTID_S, MQTTCLIENT_PERSISTENCE_NONE, NULL))
			!= MQTTASYNC_SUCCESS)
	{
		printf("Subscriber: Failed to create client, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto exit;
	}

	// Set Callbacks
	// Uses Second Argument To Store The Context (State) | In This Case, Only The Client Itself
	if ((rc = MQTTAsync_setCallbacks(client, client, connlost, msgarrvd_subscribe, NULL)) != MQTTASYNC_SUCCESS)
	{
		printf("Subscriber: Failed to set callbacks, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}

	// Connection Parameters
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1; // Persistance
	conn_opts.onSuccess = onConnect_subscribe; // Calls onConnect If Connect Successfuly
	conn_opts.onFailure = onConnectFailure_subscribe; // Calls onConnectFailure If Connection Fails
	conn_opts.context = client;

	// Connect To Broker
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Subscriber: Failed to start connect, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}

	// Wait Subscription Or Errors
	while (!subscribed && !finished_subscribe)
		#if defined(_WIN32)
			Sleep(100);
		#else
			usleep(10000L);
		#endif

	if (finished_subscribe)
		goto exit;

	// Do Nothing Until User Input "Q"/"q" (Then Quit)
	do 
	{
		ch = getchar();
	} while (ch!='Q' && ch != 'q');

	// Disconnection Parameters
	disc_opts.onSuccess = onDisconnect_subscribe;
	disc_opts.onFailure = onDisconnectFailure_subscribe;

	// Disconnect To Broker
	if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Subscriber: Failed to start disconnect, return code %d\n", rc);
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
