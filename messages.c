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

char* listGetLast(LinkedList* list) {
    if (!list) return NULL;

    pthread_mutex_lock(&list->lock);

    Node* curr = list->head;
    if (!curr) {
        pthread_mutex_unlock(&list->lock);
        return NULL;
    }

    while (curr->next) curr = curr->next;

    char* result = malloc(strlen(curr->message) + 1);
    if (result) strcpy(result, curr->message);

    pthread_mutex_unlock(&list->lock);
    return result;
}

char* listPopLast(LinkedList* list) {
    if (!list) return NULL;

    pthread_mutex_lock(&list->lock);

    Node* curr = list->head;
    if (!curr) { // empty
        pthread_mutex_unlock(&list->lock);
        return NULL;
    }

    if (!curr->next) { // single node
        char* result = malloc(strlen(curr->message) + 1);
        if (result) strcpy(result, curr->message);
        free(curr);
        list->head = NULL;
        pthread_mutex_unlock(&list->lock);
        return result;
    }

    Node* prev = curr;
    curr = curr->next;
    while (curr->next) {
        prev = curr;
        curr = curr->next;
    }

    // curr is last, prev is second-last
    char* result = malloc(strlen(curr->message) + 1);
    if (result) strcpy(result, curr->message);

    prev->next = NULL;
    free(curr);

    pthread_mutex_unlock(&list->lock);
    return result;
}

void listPopPrintAll(LinkedList* list) {
    if (!list) return;

    char* msg;
    while ((msg = listPopLast(list)) != NULL) {
        printf("%s\n", msg);
        free(msg);
    }
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
    if (!list) return 0;

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

        if (request_type)
        {
            if (strstr(request_type, "GROUP_CREATED") != NULL)
            {
                // request_body = [GROUPNAME];[TOPIC]
                char* request_group = strtok(request_body, ";");
                char* request_topic = strtok(NULL, ";");
                printf("- Você Criou O Grupo: %s.", request_group);
            }
            if (strstr(request_type, "USER_REQUEST_SENT") != NULL)
            {
                // request_body = [USERNAME]
                printf("- Você Enviou Uma Solicitação De Conversa Para: %s.", request_body);
            }
            if (strstr(request_type, "GROUP_REQUEST_SENT") != NULL)
            {
                // request_body = [GROUPNAME];[USERNAME]
                char* request_group = strtok(request_body, ";");
                char* request_username = strtok(NULL, ";");
                printf("- Você Enviou Uma Solicitação De Entrada No Grupo: %s. Possuindo O Líder: %s.", request_group, request_username);
            }
            if (strstr(request_type, "USER_REQUEST_ACCEPTED") != NULL)
            {
                // request_body = [USERNAME];[TOPIC]
                char* request_username = strtok(request_body, ";");
                printf("- Solicitação De Conversa Para: %s. Aceita!", request_username);
            }
            if (strstr(request_type, "GROUP_REQUEST_ACCEPTED") != NULL)
            {
                // request_body = [GROUPNAME];[USERNAME];[TOPIC]
                char* request_groupname = strtok(request_body, ";");
                char* request_username = strtok(NULL, ";");
                printf("- Solicitação De Entrada No Grupo: %s. Possuindo O Líder: %s. Aceita!", request_groupname, request_username);
            }
            if (strstr(request_type, "USER_REQUEST_REJECTED") != NULL)
            {
                // request_body = [USERNAME]
                printf("- Solicitação De Conversa Para: %s. Rejeitada.", request_body);
            }
            if (strstr(request_type, "GROUP_REQUEST_REJECTED") != NULL)
            {
                // request_body = [GROUPNAME];[USERNAME]
                char* request_groupname = strtok(request_body, ";");
                char* request_username = strtok(NULL, ";");
                printf("- Solicitação De Entrada No Grupo: %s. Possuindo O Líder: %s. Rejeitada.", request_groupname, request_username);
            }
            if (strstr(request_type, "USER_ACCEPTED") != NULL)
            {
                // request_body = [USERNAME];[TOPIC]
                char* request_username = strtok(request_body, ";");
                printf("- Você Aceitou A Solicitação De Conversa Com O Usuário: %s. ", request_username);
            }
            if (strstr(request_type, "GROUP_ACCEPTED") != NULL)
            {
                // request_body = [GROUPNAME];[USERNAME]
                char* request_groupname = strtok(request_body, ";");
                char* request_username = strtok(NULL, ";");
                printf("- Você Aceitou A Solicitação De Entrada No Grupo: %s. Pelo Usuário: %s.", request_groupname, request_username);
            }
            if (strstr(request_type, "USER_REJECTED") != NULL)
            {
                // request_body = [USERNAME]
                printf("- Você Rejeitou A Solicitação De Conversa Com O Usuário: %s.", request_body);
            }
            if (strstr(request_type, "GROUP_REJECTED") != NULL)
            {
                // request_body = [GROUPNAME];[USERNAME]
                char* request_groupname = strtok(request_body, ";");
                char* request_username = strtok(NULL, ";");
                printf("- Você Rejeitou A Solicitação De Entrada No Grupo: %s. Pelo Usuário: %s.", request_groupname, request_username);
            }
        }

        printf("\n");

        curr = curr->next;
    }

    pthread_mutex_unlock((pthread_mutex_t*)&list->lock);
}

void listPrintChats(const LinkedList* list, const char *username) {
    if (!list) return;

    pthread_mutex_lock((pthread_mutex_t*)&list->lock);

    Node* curr = list->head;
    while (curr) {

        char message[1024];
        strncpy(message, curr->message, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';  // Ensure Null Termination

        char* request_type = strtok(message, ":"); // (constants.h)
        char* request_body = strtok(NULL, ":"); // (constants.h)

        if (request_type)
        {
            if (strstr(request_type, "GROUP_CREATED") != NULL || strstr(request_type, "GROUP_REQUEST_ACCEPTED") != NULL)
            {
                // request_body = [GROUPNAME];[TOPIC]
                char* group = strtok(request_body, ";");
                printf("- Grupo: %s\n", group);
            }
            if ((strstr(request_type, "USER_REQUEST_ACCEPTED") != NULL || strstr(request_type, "USER_ACCEPTED") != NULL) && request_body != NULL)
            {
                // request_body = [USERNAME];[TOPIC]
                char* user = strtok(request_body, ";");
                printf("- Usuário: %s\n", user);
            }
        }

        curr = curr->next;
    }

    printf("\n");

    pthread_mutex_unlock((pthread_mutex_t*)&list->lock);
}

char* listGetGroup(const LinkedList* groups_list, const char* group, const char* leader)
{
    pthread_mutex_lock((pthread_mutex_t*)&groups_list->lock);

    char formmated_target[512];
    char formmated_list[512];
    snprintf(formmated_target, sizeof(formmated_target), "%s:%s", group, leader);

    Node* curr = groups_list->head;
    while (curr) {
        char message[1024];
        strncpy(message, curr->message, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';

        char* list_group = strtok(message, ":");
        char* list_leader = strtok(NULL, ":");

        if (list_group && list_leader)
        {
            snprintf(formmated_list, sizeof(formmated_list), "%s:%s", list_group, list_leader);

            if (strcmp(formmated_target, formmated_list) == 0)
            {
                char* result = malloc(strlen(curr->message) + 1);
                strcpy(result, curr->message);
                pthread_mutex_unlock((pthread_mutex_t*)&groups_list->lock);
                return result;
            }

            curr = curr->next;
        }
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
            char* result = malloc(strlen(leader) + 1);
            strcpy(result, leader);
            pthread_mutex_unlock((pthread_mutex_t*)&groups_list->lock);
            return result;
        }

        curr = curr->next;
    }

    pthread_mutex_unlock((pthread_mutex_t*)&groups_list->lock);
}

char* listGetTopic(const LinkedList* history_list, const char* target)
{
    pthread_mutex_lock((pthread_mutex_t*)&history_list->lock);

    Node* curr = history_list->head;
    while (curr) {
        char message[1024];
        strncpy(message, curr->message, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';

        char* type = strtok(message, ":");

        if (type)
        {
            if (strcmp(type, "GROUP_CREATED") != 0 && strcmp(type, "GROUP_REQUEST_ACCEPTED") != 0 && strcmp(type, "USER_ACCEPTED") != 0 && strcmp(type, "USER_REQUEST_ACCEPTED") != 0)
            {
                curr = curr->next;
                continue;
            }

            if (strcmp(type, "GROUP_CREATED") == 0 || strcmp(type, "USER_ACCEPTED") == 0 || strcmp(type, "USER_REQUEST_ACCEPTED") == 0)
            {
                char* name = strtok(NULL, ";");
                char* topic = strtok(NULL, ";");
                
                if (name && topic)
                {
                    if (strcmp(name, target) == 0)
                    {
                        char* result = malloc(strlen(topic) + 1);
                        strcpy(result, topic);
                        pthread_mutex_unlock((pthread_mutex_t*)&history_list->lock);
                        return result;
                    }
                }
            }
            else
            {
                char* groupname = strtok(NULL, ";");
                char* leader = strtok(NULL, ";");
                char* topic = strtok(NULL, ";");
                
                if (groupname && leader && topic)
                {
                    if (strcmp(groupname, target) == 0)
                    {
                        char* result = malloc(strlen(topic) + 1);
                        strcpy(result, topic);
                        pthread_mutex_unlock((pthread_mutex_t*)&history_list->lock);
                        return result;
                    }
                }
            }

            curr = curr->next; 
        }
    }

    pthread_mutex_unlock((pthread_mutex_t*)&history_list->lock);
    return NULL;
}

int listSearchFirstParameter(LinkedList* list, const char* target) {
    pthread_mutex_lock(&list->lock);

    Node* curr = list->head;
    int found = 0;

    while (curr) {
        char message[1024];
        strncpy(message, curr->message, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';

        char* first = strtok(message, ":");

        if (first && strcmp(first, target) == 0) {
            found = 1;
            break;
        }
        curr = curr->next;
    }

    pthread_mutex_unlock(&list->lock);
    return found;
}

int listSearchConversation(LinkedList* list, const char* target) {
    if (!list || !target) return 0;

    pthread_mutex_lock(&list->lock);

    Node* curr = list->head;
    int found = 0;
    while (curr) {
        char message[1024];
        strncpy(message, curr->message, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';

        char* type = strtok(message, ":");
        if (!type) { curr = curr->next; continue; }

        if (strcmp(type, "GROUP_CREATED") != 0 && strcmp(type, "GROUP_REQUEST_ACCEPTED") != 0 &&
            strcmp(type, "USER_ACCEPTED") != 0 && strcmp(type, "USER_REQUEST_ACCEPTED") != 0) {
            curr = curr->next;
            continue;
        }

        char* name = strtok(NULL, ";");
        if (name && strcmp(name, target) == 0) {
            found = 1;
            break;
        }

        curr = curr->next;
    }

    pthread_mutex_unlock(&list->lock);
    return found;
}

int listSearchChat(LinkedList* list) {
    if (!list) return 0;

    pthread_mutex_lock(&list->lock);

    Node* curr = list->head;
    int found = 0;

    while (curr) {
        char message[1024];
        strncpy(message, curr->message, sizeof(message) - 1);
        message[sizeof(message) - 1] = '\0';

        char* type = strtok(message, ":");
        if (type && (
            strcmp(type, "GROUP_CREATED") == 0 ||
            strcmp(type, "GROUP_REQUEST_ACCEPTED") == 0 ||
            strcmp(type, "USER_ACCEPTED") == 0 ||
            strcmp(type, "USER_REQUEST_ACCEPTED") == 0))
        {
            found = 1;
            break;
        }

        curr = curr->next;
    }

    pthread_mutex_unlock(&list->lock);
    return found;
}