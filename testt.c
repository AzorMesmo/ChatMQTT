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

//connection parameters
#define ADDRESS     "tcp://test.mosquitto.org:1883"
#define CLIENTID    "ExampleClientPub"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L

const char* TOPIC;  // tópico a ser usado


int disc_finished = 0; //disconnection finished flag
int subscribed = 0; //subscription finished flag
int finished = 0; //program finished flag

void onSubscribe(void* context, MQTTAsync_successData* response);
void onSubscribeFailure(void* context, MQTTAsync_failureData* response);


// Callbacks - chamadas qundo o cliente desconecta do broker
void onDisconnectFailure(void* context, MQTTAsync_failureData* response) {
    printf("\nDisconnect failed\n");
    if (response->code)
        printf("     code: %d\n", response->code);
    disc_finished = 1;
}

void onDisconnect(void* context, MQTTAsync_successData* response) {
    printf("Successful disconnection\n");
    disc_finished = 1;
}

// Callback - chamada quando a mensagem falha ao ser enviada
void onSendFailure(void* context, MQTTAsync_failureData* response) {
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
    int rc;

    printf("Message send failed token %d error code %d\n", response->token, response->code);

    opts.onSuccess = onDisconnect;
    opts.onFailure = onDisconnectFailure;
    opts.context = client;

    if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start disconnect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
}

// Callback - chamada quando a mensagem é enviada com sucesso
void onSend(void* context, MQTTAsync_successData* response) {
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;// tenta desconectar do broker p limpar sessao
    int rc;

    printf("Message with token value %d delivery confirmed\n", response->token);

    opts.onSuccess = onDisconnect;
    opts.onFailure = onDisconnectFailure;
    opts.context = client;

    if ((rc = MQTTAsync_disconnect(client, &opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start disconnect, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
}

// Callback - chamada quando a conexão é bem sucedida
void onConnect(void* context, MQTTAsync_successData* response) {
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	MQTTAsync_responseOptions sub_opts = MQTTAsync_responseOptions_initializer;
    int rc;

    printf("Successful connection\n");

    opts.onSuccess = onSend;
    opts.onFailure = onSendFailure;
    opts.context = client;

    pubmsg.payload = PAYLOAD;
    pubmsg.payloadlen = (int)strlen(PAYLOAD); //mensagem a ser publicada
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    if ((rc = MQTTAsync_sendMessage(client, TOPIC, &pubmsg, &opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start sendMessage, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }

	// assinar o mesmo tópico (ou outro tópico)
    sub_opts.onSuccess = onSubscribe;
    sub_opts.onFailure = onSubscribeFailure;
    sub_opts.context = client;

    if ((rc = MQTTAsync_subscribe(client, TOPIC, QOS, &sub_opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start subscribe, return code %d\n", rc);
        exit(EXIT_FAILURE);
    }
}

// Callback - chamada quando a conexão falha
void onConnectFailure(void* context, MQTTAsync_failureData* response) {
    printf("Connection failed, rc %d\n", response->code);
    finished = 1; // termina o programa
}

// Callback - chamada quando a conexão é perdida
void connlost(void *context, char *cause) {
    MQTTAsync client = (MQTTAsync)context;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    int rc;

    printf("\nConnection lost\n");
    if (cause) printf("     cause: %s\n", cause);
    printf("Reconnecting\n");

    conn_opts.keepAliveInterval = 30;
    conn_opts.cleansession = 1;
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;

    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start connect, return code %d\n", rc);
        finished = 1;
    }
}

// Callback - chamada quando uma mensagem chega
int msgarrvd(void *context, char *topicName, int topicLen, MQTTAsync_message *message) {
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: %.*s\n", message->payloadlen, (char*)message->payload);// imprime a mensagem recebida

    MQTTAsync_freeMessage(&message);
    MQTTAsync_free(topicName); // libera a memoria alocada para o nome do topico
    return 1;
}

// Callback - atualiza flag quando a inscricao é bem sucedida
void onSubscribe(void* context, MQTTAsync_successData* response) {
    printf("Subscribe succeeded\n");
    subscribed = 1;
}

// Callback - atualiza flag quando a inscricao falha
void onSubscribeFailure(void* context, MQTTAsync_failureData* response) {
    printf("Subscribe failed, rc %d\n", response->code);
    finished = 1;
}

// publica mensagem no topico
void publishMessage(MQTTAsync client, const char* payload) {
    MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
    MQTTAsync_message pubmsg = MQTTAsync_message_initializer;

    opts.onSuccess = onSend;
    opts.onFailure = onSendFailure;
    opts.context = client;

    pubmsg.payload = (char*)payload;
    pubmsg.payloadlen = (int)strlen(payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

	MQTTAsync_sendMessage(client, TOPIC, &pubmsg, &opts); // envia a mensagem escrita no main
}

//////////////////// MAIN //////////////////////

int main(int argc, char* argv[]) {
    MQTTAsync client;
    MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
    MQTTAsync_disconnectOptions disc_opts = MQTTAsync_disconnectOptions_initializer;
    int rc, ch;
	
	//define topico ex:- ./main "meu/topico/teste"
	TOPIC = (argc > 2) ? argv[2] : "MQTT Examples"; 

	const char* uri = (argc > 1) ? argv[1] : ADDRESS;    
	printf("Using server at %s\n", uri);

	// cria o cliente MQTT
    if ((rc = MQTTAsync_create(&client, uri, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL)) != MQTTASYNC_SUCCESS) { //MQTTCLIENT_PERSISTENCE_NONE: sem persistencia
        printf("Failed to create client, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto exit;
    }

	// define os callbacks
    if ((rc = MQTTAsync_setCallbacks(client, client, connlost, msgarrvd, NULL)) != MQTTASYNC_SUCCESS) { //NULL para onDeliveryComplete, mensagens QoS nao tem confirmacao de entrega
        printf("Failed to set callbacks, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }

    conn_opts.keepAliveInterval = 20; // intervalo de keep alive
    conn_opts.cleansession = 1; // limpa a sessao anterior
    conn_opts.onSuccess = onConnect;
    conn_opts.onFailure = onConnectFailure;
    conn_opts.context = client;

	// conecta ao broker
    if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start connect, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }

    while (!subscribed && !finished) // espera ate a inscricao ser concluida
		#if defined(_WIN32)
				Sleep(100);
		#else
				usleep(10000L);
		#endif

    if (finished) goto exit;

	char message[256];
	while (1) {
		printf("Digite a mensagem (ou 'Q' para sair): ");
		scanf(" %[^\n]", message); // lê até o usuário apertar Enter, aceita espaços

		if (message[0] == 'Q' || message[0] == 'q') 
			break;

		publishMessage(client, message);
	}


    do { // espera o usuario pressionar 'Q' ou 'q' para sair
        ch = getchar();
    } while (ch != 'Q' && ch != 'q');

    disc_opts.onSuccess = onDisconnect;
    disc_opts.onFailure = onDisconnectFailure;

	// desconecta do broker
    if ((rc = MQTTAsync_disconnect(client, &disc_opts)) != MQTTASYNC_SUCCESS) {
        printf("Failed to start disconnect, return code %d\n", rc);
        rc = EXIT_FAILURE;
        goto destroy_exit;
    }

    while (!disc_finished) // espera ate a desconexao ser concluida
		#if defined(_WIN32)
				Sleep(100);
		#else
				usleep(10000L);
		#endif

destroy_exit:
    MQTTAsync_destroy(&client);
exit:
    return rc;
}
