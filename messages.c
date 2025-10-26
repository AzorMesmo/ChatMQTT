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
        perror("malloc failed");
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

    pthread_mutex_lock((pthread_mutex_t*)&list->lock); // cast away const for locking

    Node* curr = list->head;
    while (curr) {
        printf("%s\n", curr->message);
        curr = curr->next;
    }

    pthread_mutex_unlock((pthread_mutex_t*)&list->lock);
}

// Advanced Functions

// Update Status (Only Insert In The List If It's Not Duplicated. If Status Is Different, Replace It)
void listInsertStatus(LinkedList* list, const char* message) {
    pthread_mutex_lock(&list->lock);

    // Extract Username (Before ':')
    char username[256];
    const char* colon = strchr(message, ':');

    // If Don't Have ':' > Invalid Format
    if (!colon) {
        fprintf(stderr, "Invalid message format: %s\n", message);
        pthread_mutex_unlock(&list->lock);
        return;
    }

    // Get Username
    size_t len = colon - message;
    if (len >= sizeof(username))
        len = sizeof(username) - 1;
    strncpy(username, message, len);
    username[len] = '\0'; // Need To Add The Null Terminator Because Of strncpy

    // Search For Existing Message From Same User
    Node *curr = list->head, *prev = NULL;
    while (curr) {
        // (curr->message[len] == ':') > Avoid Miss Matching Usernames Like "Examp" And "Example"
        if (strncmp(curr->message, username, len) == 0 && curr->message[len] == ':') {
            // Found Username
            if (strcmp(curr->message, message) == 0) {
                // Same Status > Do Nothing
                pthread_mutex_unlock(&list->lock);
                return;
            }
            // Different Status > Replace Message
            strncpy(curr->message, message, sizeof(curr->message) - 1);
            curr->message[sizeof(curr->message) - 1] = '\0'; // Need To Add The Null Terminator Because Of strncpy
            pthread_mutex_unlock(&list->lock);
            return;
        }
        prev = curr;
        curr = curr->next;
    }

    // New User (No Messages) > Insert New Node
    Node* new_node = malloc(sizeof(Node));
    if (!new_node) {
        perror("Status List Node: Malloc Failed");
        pthread_mutex_unlock(&list->lock);
        return;
    }
    strncpy(new_node->message, message, sizeof(new_node->message) - 1);
    new_node->message[sizeof(new_node->message) - 1] = '\0'; // Need To Add The Null Terminator Because Of strncpy
    new_node->next = list->head;
    list->head = new_node;
    pthread_mutex_unlock(&list->lock);
}
