// Compilation Command: "gcc main.c -o main -lpaho-mqtt3as -pthread"
// Excecution Command: "./main"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "publisher.c"
#include "subscriber.c"

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

// Thread Function Wrappers

void* statusHeartbeat(void* arg) // Keep Sending Status To USERS Topic (publishStatus)
{
    PublishArgs* args = (PublishArgs*)arg;

    while (online)
    {
        publishStatus(args->username, args->topic, args->payload);

        #if defined(_WIN32)
			Sleep(30000); // 30 Seconds
		#else
			usleep(30000000L); // 30 Seconds
		#endif

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

int main()
{
    // Welcome & Username Definition

    char username[64];
    printf("ChatMQTT\n\n\nDigite Seu Nome De Usuário (Máximo 63 Caractéres): ");
    scanf("%63s", username); // Limit Username Input To 63 Characters + '\0'
    printf("Bem Vindo, %s!\n\n\n", username);

    // Threads Parameters

    pthread_t threads[1]; // Threads Handler
    int threads_running = 0; // Threads Counter

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

    // Monitor Users Status
    // WIP: A thread to aways save all messages from USERS since program startup, all users send their status within 30 seconds, so in 30 seconds
    // we will have all user status. We just have to keep replacing the duplicated status.

    // pthread_t threads[2]; // Thread Handler (Publisher / Subscriber)

    // // Create Threads
    // if (pthread_create(&threads[0], NULL, main_subscribe, NULL) != 0) // Subscriber
    // {
    //     perror("Failed To Create The Subscriber");
    //     return 1;
    // }
    // if (pthread_create(&threads[0], NULL, publishStatus_thread, NULL) != 0) // Publisher
    // {
    //     perror("Failed To Create The Publisher");
    //     return 1;
    // }

    #if defined(_WIN32)
			Sleep(65000);
		#else
			usleep(65000000L);
		#endif

    online = 0;

    // Wait For Threads Completion
    for (int i = 0; i < threads_running; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Send Offline Status (USERS Topic)
    snprintf(payload, sizeof(payload), "%s: Offline", username);
    publishStatus(username, "USERS", payload);

    return 0;
}