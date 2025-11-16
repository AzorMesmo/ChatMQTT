// Messages Queue

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include "constants.h"
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
        perror("List Node Malloc Failed");
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

void toUppercase(char *str)
{
    for (int i = 0; str[i] != '\0'; i++)
        str[i] = toupper((unsigned char)str[i]);
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

void listPrintRequests(const LinkedList* list) {
    if (!list) return;

    pthread_mutex_lock((pthread_mutex_t*)&list->lock);

    Node* curr = list->head;
    while (curr) {
        char message[1024];
        strncpy(message, curr->message, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';  // Ensure Null Termination

        char* request_type = strtok(message, ":"); // Request Type (USER_REQUEST | GROUP_REQUEST)
        char* request_body = strtok(NULL, ":"); // User (Sender)

        printf("\n");

        if (request_type && strstr(request_type, "USER_REQUEST") != NULL)
        {
            // request_body = [USERNAME]
            printf("- Solicitação De Conversa Com: %s.", request_body);
        }
        if (request_type && strstr(request_type, "GROUP_REQUEST") != NULL)
        {
            // request_body = [GROUP];[USERNAME]
            char* request_group = strtok(request_body, ";");
            char* request_username = strtok(NULL, ";");
            printf("- Solicitação De Entrada No Grupo: %s. Pelo Usuário: %s.", request_group, request_username);
        }

        printf("\n");

        curr = curr->next;
    }

    pthread_mutex_unlock((pthread_mutex_t*)&list->lock);
}

void listPrintHistory(const LinkedList* list, const char *username) {
    if (!list) return;

    pthread_mutex_lock((pthread_mutex_t*)&list->lock);

    Node* curr = list->head;
    while (curr) {

        char message[1024];
        strncpy(message, curr->message, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';  // Ensure Null Termination

        char* request_type = strtok(message, ":"); // (constants.h)
        char* request_body = strtok(NULL, ":"); // (constants.h)

        printf("\n");

        if (request_type && strstr(request_type, "GROUP_CREATED") != NULL)
        {
            // request_body = [GROUPNAME];[TOPIC]
            char* request_group = strtok(request_body, ";");
            char* request_topic = strtok(NULL, ";");
            printf("- Você Criou O Grupo: %s.", request_group);
        }
        if (request_type && strstr(request_type, "USER_REQUEST_SENT") != NULL)
        {
            // request_body = [USERNAME]
            printf("- Você Enviou Uma Solicitação De Conversa Para: %s.", request_body);
        }
        if (request_type && strstr(request_type, "GROUP_REQUEST_SENT") != NULL)
        {
            // request_body = [GROUPNAME];[USERNAME]
            char* request_group = strtok(request_body, ";");
            char* request_username = strtok(NULL, ";");
            printf("- Você Enviou Uma Solicitação De Entrada No Grupo: %s. Possuindo O Líder: %s.", request_group, request_username);
        }

        printf("\n");

        curr = curr->next;
    }

    pthread_mutex_unlock((pthread_mutex_t*)&list->lock);
}

void listGetOnlineUsers(const LinkedList* list, LinkedList* online_list, const char* username) {
    listClear(online_list);

    pthread_mutex_lock((pthread_mutex_t*)&list->lock);

    Node* curr = list->head;
    while (curr) {
        char message[1024];
        strncpy(message, curr->message, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';

        char* user = strtok(message, ":");
        char* status = strtok(NULL, ":");

        if (user && status && strcmp(status, "Online") == 0 && strcmp(user, username) != 0) {
            listInsert(online_list, user);
        }

        curr = curr->next;
    }

    pthread_mutex_unlock((pthread_mutex_t*)&list->lock);
}

void listGetOnlineGroups(LinkedList* online_users_list, const LinkedList* groups_list, LinkedList* online_groups_list, const char* username)
{
    listClear(online_groups_list);

    pthread_mutex_lock((pthread_mutex_t*)&groups_list->lock);

    Node* curr = groups_list->head;
    while (curr) {
        char message[1024];
        strncpy(message, curr->message, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';

        char* groupname = strtok(message, ":"); // Group Name
        char* leader = strtok(NULL, ":"); // Group Leader
        char* members = strtok(NULL, ":"); // Group Members

        if (groupname && leader && members && strcmp(leader, username) != 0)
        {
            if (listSearch(online_users_list, leader) == 1)
            {
                listInsert(online_groups_list, curr->message);
            }
        }

        curr = curr->next;
    }

    pthread_mutex_unlock((pthread_mutex_t*)&groups_list->lock);
}

char* listGetGroupLeader(const LinkedList* groups_list, const char* group)
{
    pthread_mutex_lock((pthread_mutex_t*)&groups_list->lock);

    Node* curr = groups_list->head;
    while (curr) {
        char message[1024];
        strncpy(message, curr->message, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';

        char* groupname = strtok(message, ":"); // Group Name
        char* leader = strtok(NULL, ":"); // Group Leader
        char* members = strtok(NULL, ":"); // Group Members

        if (groupname && leader && strcmp(groupname, group) == 0)
        {
            return leader;
        }

        curr = curr->next;
    }

    pthread_mutex_unlock((pthread_mutex_t*)&groups_list->lock);
}

int listSearchFirstParameter(LinkedList* list, const char* message) {
    pthread_mutex_lock(&list->lock);

    Node* curr = list->head;
    int found = 0;
    while (curr) {
        char message[1024];
        strncpy(message, curr->message, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';

        char* groupname = strtok(message, ":"); // First Parameter

        if (strcmp(groupname, message) == 0) {
            found = 1;
            break;
        }
        curr = curr->next;
    }

    pthread_mutex_unlock(&list->lock);
    return found;
}