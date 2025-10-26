// Compilation Command: "gcc main.c publisher.c subscriber.c messages.c -o main -lpaho-mqtt3as -pthread"
// Excecution Command: "./main"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "publisher.h"
#include "subscriber.h"
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

volatile int online = 1;

// Thread Function Arguments

typedef struct // Publish Arguments
{
    char username[64];
    char topic[64];
    char payload[1024];
} PublishArgs;

typedef struct // Subscriber Arguments
{
    char username[64];
    char topic[64];
	LinkedList* message_list;
} SubscribeArgs;

// Thread Function Wrappers

void* statusHeartbeat(void* arg) // Keep Sending Status To USERS Topic (publisherStatus)
{
    PublishArgs* args = (PublishArgs*)arg;

    while (online)
    {
        publisherStatus(args->username, args->topic, args->payload);

        for (int i = 0; i < 3000 && online; i++) { // 10ms * 3000 = 30s
            #if defined(_WIN32)
                Sleep(10); // 10ms
            #else
                usleep(10000L); // 10ms
            #endif
        }
    }

    return NULL;
}

// Main Function

int main()
{
    // ----- Program Startup -----

    // Welcome & Username Definition

    char username[64];
    printf("\nChatMQTT\n\nDigite Seu Nome De Usuário (Máximo 63 Caractéres): ");
    scanf("%63s", username); // Limit Username Input To 63 Characters + '\0'
    printf("\nBem Vindo, %s!\n\n", username);

    printf("---------- PROGRAM LOG ----------\n\n", username);

    // Threads Parameters

    pthread_t threads[1]; // Threads Handler
    // Total Threads Number Is Based On The Maximum Possible Concurrent Threads:
    // Messages Queue (WIP)
    // Status Publisher
    // Status Subscriber (WIP)
    int threads_running = 0; // Threads Counter

    // Queues Initialization
    LinkedList status_list; // Status List
    listInit(&status_list);

    // Send Online Status (USERS Topic)

    PublishArgs pubArgs; // Function statusHeartbeat Arguments
    strncpy(pubArgs.username, username, 64); // Username
    strncpy(pubArgs.topic, "USERS", 64); // Topic
    char payload[1024];
    snprintf(payload, sizeof(payload), "%s: Online", username);
    strncpy(pubArgs.payload, payload, 1024); // Payload

    threads_running += 1;
    if (pthread_create(&threads[0], NULL, statusHeartbeat, &pubArgs) != 0) // Status Heartbeat
    {
        perror("Failed To Create The Subscriber");
        threads_running -= 1;
        return 1;
    }

    #if defined(_WIN32)
			Sleep(2500);
		#else
			usleep(2500000L);
		#endif

    printf("\n---------- PROGRAM LOG ----------\n\n");

    // ----- Program Menu -----

    printf("Digite Qualquer Coisa Para Escanear Por Usuários...\n\n");
    scanf(" %*c"); // Wait For Any Key Press
    printf("\n");

    printf("---------- PROGRAM LOG ----------\n\n");

    // Monitor Users Status
    char status_username[64]; // Create An Alternative Username To Avoid Conflict With statusHeartbeat
    snprintf(status_username, sizeof(status_username), "%s_status", username);

    subscriberStatus(status_username, "USERS", &status_list);
    
    printf("\n---------- PROGRAM LOG ----------\n\n");

    printf("> Usuário : Status <\n\n");

    listPrint(&status_list);

    printf("\n---------- PROGRAM LOG ----------\n\n");

    // pthread_t threads[2]; // Thread Handler (Publisher / Subscriber)

    // // Create Threads
    // if (pthread_create(&threads[0], NULL, main_subscribe, NULL) != 0) // Subscriber
    // {
    //     perror("Failed To Create The Subscriber");
    //     return 1;
    // }
    // if (pthread_create(&threads[0], NULL, publisherStatus_thread, NULL) != 0) // Publisher
    // {
    //     perror("Failed To Create The Publisher");
    //     return 1;
    // }

   #if defined(_WIN32)
			Sleep(10000);
		#else
			usleep(10000000L);
		#endif

    online = 0;

    // Wait For Threads Completion
    for (int i = 0; i < threads_running; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Send Offline Status (USERS Topic)
    snprintf(payload, sizeof(payload), "%s: Offline", username);
    publisherStatus(username, "USERS", payload);

    // End

    printf("\n---------- PROGRAM LOG ----------\n\n");

    printf("Até Mais, %s!\n\n", username);

    return 0;
}