// gcc main.c -o main -lpaho-mqtt3as -pthread

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "MQTTAsync_publish.c"
#include "MQTTAsync_subscribe.c"

int main()
{
    pthread_t threads[2]; // Thread Handler (Publisher / Subscriber)

    // Create Threads
    if (pthread_create(&threads[0], NULL, main_subscribe, NULL) != 0) // Subscriber
    {
        perror("Failed To Create The Subscriber");
        return 1;
    }
    if (pthread_create(&threads[0], NULL, main_publish, NULL) != 0) // Publisher
    {
        perror("Failed To Create The Publisher");
        return 1;
    }

    // Wait For Threads Completion
    for (int i = 0; i < 2; i++)
    {
        pthread_join(threads[i], NULL);
    }

    return 0;
}