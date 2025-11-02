// Messages Queue

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "messages.h"

// Basic Functions

void listInit(LinkedList* list) { // Initialize List
    list->head = NULL;
    pthread_mutex_init(&list->lock, NULL);
}

void listDestroy(LinkedList* list) {
    pthread_mutex_lock(&list->lock);
    Node* curr = list->head;
    while (curr) {
        Node* temp = curr;
        curr = curr->next;
        free(temp);
    }
    pthread_mutex_unlock(&list->lock);
    pthread_mutex_destroy(&list->lock);
}

void listInsert(LinkedList* list, const char* message) {
    pthread_mutex_lock(&list->lock);

    Node* new_node = malloc(sizeof(Node));
    if (!new_node) {
        perror("               [LOG] List Node Malloc Failed");
        pthread_mutex_unlock(&list->lock);
        return;
    }

    strncpy(new_node->message, message, sizeof(new_node->message) - 1);
    new_node->message[sizeof(new_node->message) - 1] = '\0'; // Safety
    new_node->next = list->head;
    list->head = new_node;

    pthread_mutex_unlock(&list->lock);
}

void listDelete(LinkedList* list, const char* message) {
    pthread_mutex_lock(&list->lock);

    Node *curr = list->head, *prev = NULL;
    while (curr) {
        if (strcmp(curr->message, message) == 0) {
            if (prev)
                prev->next = curr->next;
            else
                list->head = curr->next;
            free(curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    pthread_mutex_unlock(&list->lock);
}

int listSearch(LinkedList* list, const char* message) {
    pthread_mutex_lock(&list->lock);

    Node* curr = list->head;
    int found = 0;
    while (curr) {
        if (strcmp(curr->message, message) == 0) {
            found = 1;
            break;
        }
        curr = curr->next;
    }

    pthread_mutex_unlock(&list->lock);
    return found;
}

void listPrint(const LinkedList* list) {
    if (!list) return;

    pthread_mutex_lock((pthread_mutex_t*)&list->lock);

    Node* curr = list->head;
    while (curr) {
        printf("%s\n", curr->message);
        curr = curr->next;
    }

    pthread_mutex_unlock((pthread_mutex_t*)&list->lock);
}

void listClear(LinkedList* list) {
    if (!list) return;

    pthread_mutex_lock(&list->lock);

    Node* curr = list->head;
    while (curr) {
        Node* tmp = curr;
        curr = curr->next;
        free(tmp);
    }
    list->head = NULL;

    pthread_mutex_unlock(&list->lock);
}

// Specifc Print Functions

void listPrintStatus(const LinkedList* list) {
    if (!list) return;

    pthread_mutex_lock((pthread_mutex_t*)&list->lock);

    printf("\n");

    Node* curr = list->head;
    while (curr) {
        char message[1024];
        strncpy(message, curr->message, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';  // Ensure Null Termination

        char* username = strtok(message, ":");  // Username
        char* status = strtok(NULL, ":");       // Status

        if (username && status) {
            printf("%s | %s\n", username, status);
        }

        curr = curr->next;
    }

    pthread_mutex_unlock((pthread_mutex_t*)&list->lock);
}

void listPrintGroups(const LinkedList* list) {
    if (!list) return;

    pthread_mutex_lock((pthread_mutex_t*)&list->lock);

    Node* curr = list->head;
    while (curr) {
        char message[1024];
        strncpy(message, curr->message, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';  // Ensure Null Termination

        char* groupname = strtok(message, ":"); // Group Name
        char* leader = strtok(NULL, ":"); // Group Leader
        char* members = strtok(NULL, ":"); // Group Members

        printf("\n");

        if (groupname) {
            printf("%s\n", groupname);
        }

        if (leader) {
            printf("Líder: %s\n", leader);
        }

        printf("Membros: ");
        if (members) {
            char *member = strtok(members, ";");
            while (member) {
                printf("%s", member);
                member = strtok(NULL, ";");
                if (member) printf(", ");
            }
        }

        printf("\n");

        curr = curr->next;
    }

    pthread_mutex_unlock((pthread_mutex_t*)&list->lock);
}