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

typedef struct { //Groups
    char name[64];
    char leader[64];
    char members[10][64]; // Exemplo: até 10 membros
    int members_count;
} Group;
Group groups[10];
int groups_count = 0;

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

//atualiza grupo quando mensagem chega no tópico GROUPS
void updateGroup(const char* payload) {
    char groupName[64], leaderName[64], membersStr[256];
    sscanf(payload, "GROUP:%63[^;];LEADER:%63[^;];MEMBERS:%255[^\n]", 
           groupName, leaderName, membersStr);

    //se o grupo já existe
    int i;
    for (i = 0; i < groups_count; i++) {
        if (strcmp(groups[i].name, groupName) == 0) break;
    }

    //cria novo grupo
    if (i == groups_count) {
        strncpy(groups[groups_count].name, groupName, 64);
        strncpy(groups[groups_count].leader, leaderName, 64);
        groups[groups_count].members_count = 0;
        groups_count++;
    }

    // atualiza membros
    char* token = strtok(membersStr, ",");
    groups[i].members_count = 0;
    while (token && groups[i].members_count < 10) {
        strncpy(groups[i].members[groups[i].members_count], token, 64);
        groups[i].members_count++;
        token = strtok(NULL, ",");
    }
}

void createGroup(const char* groupName, const char* leaderName) {
    char payload[1024];
    snprintf(payload, sizeof(payload), "GROUP:%s;LEADER:%s;MEMBERS:%s", groupName, leaderName, leaderName);

    updateGroup(payload);

    publisherStatus(leaderName, "GROUPS", payload);
    printf("Grupo '%s' criado com líder '%s'\n", groupName, leaderName);
}

void listGroups() {
    printf("Listagem dos grupos cadastrados:\n\n");

    if (groups_count == 0) {
        printf("Nenhum grupo cadastrado.\n");
        return;
    }

    for (int i = 0; i < groups_count; i++) {
        printf("Grupo: %s\n", groups[i].name);
        printf("Líder: %s\n", groups[i].leader);
        printf("Membros: ");
        if (groups[i].members_count == 0) {
            printf("Nenhum membro\n");
        } else {
            for (int j = 0; j < groups[i].members_count; j++) {
                printf("%s", groups[i].members[j]);
                if (j < groups[i].members_count - 1) printf(", ");
            }
        }
        printf("\n\n");
    }
}

//SOLICITAÇÃO P ENTRAR EM GRUPOS

void requestJoinGroup(const char* groupName, const char* username, const char* leaderName) {
    char payload[256];
    snprintf(payload, sizeof(payload), "JOIN_REQUEST;GROUP:%s;USER:%s", groupName, username);

    char leader_topic[80];
    snprintf(leader_topic, sizeof(leader_topic), "%s_Control", leaderName);
    publisherStatus(username, leader_topic, payload); // publica no tópico de controle do líder
    printf("Solicitação enviada para '%s' para entrar no grupo '%s'\n", leaderName, groupName);
}

// processa solicitações recebidas
void processGroupRequest(const char* payload, const char* username) {
    char action[32], groupName[64], requester[64];
    if (sscanf(payload, "%31[^;];GROUP:%63[^;];USER:%63s", action, groupName, requester) != 3)
        return;

    if (strcmp(action, "JOIN_REQUEST") == 0) {
        printf("\nUsuário '%s' quer entrar no grupo '%s'. Aceitar? (s/n): ", requester, groupName);
        char resp;
        scanf(" %c", &resp);
        if (resp == 's' || resp == 'S') {
            // Aceitar: adiciona membro e publica atualização
            for (int i = 0; i < groups_count; i++) {
                if (strcmp(groups[i].name, groupName) == 0) {
                    if (groups[i].members_count < 10) {
                        strncpy(groups[i].members[groups[i].members_count], requester, 64);
                        groups[i].members_count++;
                    }
                    break;
                }
            }

            //atualiza tópico GROUPS
            char membersStr[256] = "";
            for (int i = 0; i < groups_count; i++) {
                if (strcmp(groups[i].name, groupName) == 0) {
                    for (int j = 0; j < groups[i].members_count; j++) {
                        strcat(membersStr, groups[i].members[j]);
                        if (j < groups[i].members_count - 1) strcat(membersStr, ",");
                    }
                    break;
                }
            }
            char updatePayload[512];
            snprintf(updatePayload, sizeof(updatePayload), "GROUP:%s;LEADER:%s;MEMBERS:%s", groupName, username, membersStr);
            publisherStatus(username, "GROUPS", updatePayload);

            //resposta ao solicitante
            char reply[128];
            snprintf(reply, sizeof(reply), "JOIN_ACCEPT;GROUP:%s;USER:%s", groupName, username);
            char requester_topic[80];
            snprintf(requester_topic, sizeof(requester_topic), "%s_Control", requester);
            publisherStatus(username, requester_topic, reply);

            printf("Solicitação aceita e grupo atualizado.\n");
        } else {
            char reply[128];
            snprintf(reply, sizeof(reply), "JOIN_REJECT;GROUP:%s;USER:%s", groupName, username);
            publisherStatus(username, requester, reply);
            printf("Solicitação rejeitada.\n");
        }
    }
}

// processa respostas de solicitação
void processGroupResponse(const char* payload, const char* username) {
    char action[32], groupName[64], responder[64];
    if (sscanf(payload, "%31[^;];GROUP:%63[^;];USER:%63s", action, groupName, responder) != 3)
        return;

    if (strcmp(action, "JOIN_ACCEPT") == 0) {
        printf("Sua solicitação para entrar no grupo '%s' foi aceita por '%s'\n", groupName, responder);
    } else if (strcmp(action, "JOIN_REJECT") == 0) {
        printf("Sua solicitação para entrar no grupo '%s' foi rejeitada por '%s'\n", groupName, responder);
    }
}


void checkGroupRequests(const char* username) {
    LinkedList temp_list;
    listInit(&temp_list);

    SubscribeArgs controlArgs;
    strncpy(controlArgs.username, username, 64);
    snprintf(controlArgs.topic, 64, "%s_Control", username);
    controlArgs.message_list = &temp_list;

    //chama subscriber apenas uma vez para ler mensagens
    subscriberStatus(controlArgs.username, controlArgs.topic, controlArgs.message_list);

    //processa cada mensagem recebida
    Node* node = temp_list.head;
    while (node) {
        processGroupRequest(node->message, username);
        processGroupResponse(node->message, username);
        node = node->next;
    }

    //limpa a lista temporária
    Node* n = temp_list.head;
    while (n) {
        Node* tmp = n;
        n = n->next;
        free(tmp); // não precisa free(tmp->message) porque é array interno
    }
    temp_list.head = NULL;
}


void startConversation() {
    printf("Funcionalidade de conversa ainda não implementada.\n");
}

void showRequestHistory() {
    printf("Histórico.\n");
}

void listClear(LinkedList* list) {
    Node* current = list->head;
    while (current) {
        Node* tmp = current;
        current = current->next;
        free(tmp); // se message for alocado dinamicamente, faça free(tmp->message);
    }
    list->head = NULL;
}

void* subscriberControlThread(void* arg) { //nao funciona, era pra ficar verificando solicitações
    SubscribeArgs* args = (SubscribeArgs*)arg;

    while (online) {
        Node* node = args->message_list->head;
        while (node) {
            processGroupRequest(node->message, args->username);
            processGroupResponse(node->message, args->username);
            node = node->next;
        }

        listClear(args->message_list); // limpa mensagens já processadas

        #if defined(_WIN32)
            Sleep(500);
        #else
            usleep(500000L); // 0,5s
        #endif
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

    printf("---------- PROGRAM LOG ----------\n\n");

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

    LinkedList group_list;
    listInit(&group_list);

    SubscribeArgs controlArgs;
    strncpy(controlArgs.username, username, 64);
    snprintf(controlArgs.topic, 64, "%s_Control", username); // X_control
    controlArgs.message_list = &group_list;

    SubscribeArgs groupsArgs;
    strncpy(groupsArgs.username, username, 64);
    strncpy(groupsArgs.topic, "GROUPS", 64);
    groupsArgs.message_list = &group_list;

    pthread_t subThreads[2];
    pthread_create(&subThreads[0], NULL, subscriberControlThread, &controlArgs);
    pthread_create(&subThreads[1], NULL, subscriberControlThread, &groupsArgs);


    #if defined(_WIN32)
        Sleep(1000); 
    #else
        usleep(1000000L); 
    #endif



    // ----- Program Menu -----

    char op;
    char op2;
    char op3;
    char groupName[64];

    while (1) {
        printf("Menu:\n1. Listar Usuários\n2. Listar Grupos\n3. Conversar\n4. Criar Grupo\n5. Solicitações de Conversa\n6. Sair\n\n");
        scanf(" %c", &op); 

        if(op == '1'){
            printf("---------- PROGRAM LOG ----------\n\n");

            // Monitor Users Status
            char status_username[128]; // Create An Alternative Username To Avoid Conflict With statusHeartbeat
            snprintf(status_username, sizeof(status_username), "%s_status", username);

            subscriberStatus(status_username, "USERS", &status_list);
            
            printf("\n---------- PROGRAM LOG ----------\n\n");

            printf("> Usuário : Status <\n\n");

            listPrint(&status_list);

            printf("\n---------- PROGRAM LOG ----------\n\n");
        }
        else if(op == '2'){
            listGroups();
        }
        else if(op == '3'){
            printf("Conversar com:\n1. Amigos\n2. Grupos\n");
            scanf(" %c", &op2);
            startConversation();
        }
        else if(op == '4'){
            printf("Nome do Grupo: ");
            scanf("%63s", groupName);
            createGroup(groupName, username);
        }
        else if(op == '5'){
            printf("1. Minhas solicitações de entrada em grupos:\n2. Enviar solicitação de entrada em grupos:\n3. Minhas solicitação de conversa\n4. Enviar solicitações de conversa\n");
            scanf(" %c", &op3);
            if(op3 == '1'){
                printf("\n--- Verificando solicitações ---\n");
                //checkGroupRequests(username); //problem
                printf("--- Fim das solicitações ---\n\n");
            }
            else if(op3 == '2'){
                printf("Nome do Grupo: ");
                char groupName[64];
                scanf("%63s", groupName);

                printf("Nome do líder do grupo: ");
                char leader[64];
                scanf("%63s", leader);

                requestJoinGroup(groupName, username, leader);
            }
            else if(op3 == '3'){
                printf("Funcionalidade ainda não implementada.\n");
            }
            else if(op3 == '4'){
                printf("Funcionalidade não implementada.\n");
            }
            else{
                printf("Opção Inválida!\n");
            }
        }
        else if(op == '6'){
            printf("Saindo...\n");
            break;
        }
        else{
            printf("Opção Inválida!\n");
        }
    }
    printf("\n");


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