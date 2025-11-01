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

#define DELAY_5_SEC_MS 5000
#define DELAY_5_SEC_US 5000000L
#define DELAY_10_SEC_MS 10000
#define DELAY_10_SEC_US 10000000L

volatile int online = 1;

// Thread Function Arguments

typedef struct // Publish Arguments
{
    char username[64];
    char topic[128];
    char payload[1024];
} PublishArgs;

typedef struct // Subscriber Arguments
{
    char username[64];
    char topic[128];
	LinkedList* message_list;
} SubscribeArgs;

typedef struct // Groups
{ 
    char name[64];
    char leader[64];
    char members[10][64]; // Exemplo: até 10 membros
    int members_count;
} Group;

// Thread Function Wrappers

// WIP

// Default Functions

void setStatus(const char* username, const char* status) {
    char topic[80];
    char payload[128];

    snprintf(topic, sizeof(topic), "USERS/%s", username);
    snprintf(payload, sizeof(payload), "%s:%s", username, status);

    publisherStatus(username, topic, payload);
}

void getUsers(const char* username, LinkedList* status_list) {
    listClear(status_list);
    subscriberUsers(username, "USERS/+", status_list);
    printf("\n= Usuário:Status =\n");
    listPrint(status_list);
}

// //atualiza grupo quando mensagem chega no tópico GROUPS
// void updateGroup(const char* payload, int groups_count) {
//     char groupName[64], leaderName[64], membersStr[256];
//     sscanf(payload, "GROUP:%63[^;];LEADER:%63[^;];MEMBERS:%255[^\n]", 
//            groupName, leaderName, membersStr);

//     //se o grupo já existe
//     int i;
//     for (i = 0; i < groups_count; i++) {
//         if (strcmp(groups[i].name, groupName) == 0) break;
//     }

//     //cria novo grupo
//     if (i == groups_count) {
//         strncpy(groups[groups_count].name, groupName, 64);
//         strncpy(groups[groups_count].leader, leaderName, 64);
//         groups[groups_count].members_count = 0;
//         groups_count++;
//     }

//     // atualiza membros
//     char* token = strtok(membersStr, ",");
//     groups[i].members_count = 0;
//     while (token && groups[i].members_count < 10) {
//         strncpy(groups[i].members[groups[i].members_count], token, 64);
//         groups[i].members_count++;
//         token = strtok(NULL, ",");
//     }
// }

// void createGroup(const char* groupName, const char* leaderName, int groups_count) {
//     char payload[1024];
//     snprintf(payload, sizeof(payload), "GROUP:%s;LEADER:%s;MEMBERS:%s", groupName, leaderName, leaderName);

//     updateGroup(payload, groups_count);

//     publisherStatus(leaderName, "GROUPS", payload);
//     printf("Grupo '%s' criado com líder '%s'\n", groupName, leaderName);
// }

// void listGroups() {
//     printf("Listagem dos grupos cadastrados:\n\n");

//     if (groups_count == 0) {
//         printf("Nenhum grupo cadastrado.\n");
//         return;
//     }

//     for (int i = 0; i < groups_count; i++) {
//         printf("Grupo: %s\n", groups[i].name);
//         printf("Líder: %s\n", groups[i].leader);
//         printf("Membros: ");
//         if (groups[i].members_count == 0) {
//             printf("Nenhum membro\n");
//         } else {
//             for (int j = 0; j < groups[i].members_count; j++) {
//                 printf("%s", groups[i].members[j]);
//                 if (j < groups[i].members_count - 1) printf(", ");
//             }
//         }
//         printf("\n\n");
//     }
// }

// //SOLICITAÇÃO P ENTRAR EM GRUPOS
// void requestJoinGroup(const char* groupName, const char* username, const char* leaderName) {
//     char payload[256];
//     snprintf(payload, sizeof(payload), "JOIN_REQUEST;GROUP:%s;USER:%s", groupName, username);

//     char leader_topic[80];
//     snprintf(leader_topic, sizeof(leader_topic), "%s_Control", leaderName);
//     publisherStatus(username, leader_topic, payload); // publica no tópico de controle do líder
//     printf("Solicitação enviada para '%s' para entrar no grupo '%s'\n", leaderName, groupName);
// }

// // processa solicitações recebidas
// void processGroupRequest(const char* payload, const char* username) {
//     char action[32], groupName[64], requester[64];
//     if (sscanf(payload, "%31[^;];GROUP:%63[^;];USER:%63s", action, groupName, requester) != 3)
//         return;

//     if (strcmp(action, "JOIN_REQUEST") == 0) {
//         printf("\nUsuário '%s' quer entrar no grupo '%s'. Aceitar? (s/n): ", requester, groupName);
//         char resp;
//         scanf(" %c", &resp);
//         if (resp == 's' || resp == 'S') {
//             // Aceitar: adiciona membro e publica atualização
//             for (int i = 0; i < groups_count; i++) {
//                 if (strcmp(groups[i].name, groupName) == 0) {
//                     if (groups[i].members_count < 10) {
//                         strncpy(groups[i].members[groups[i].members_count], requester, 64);
//                         groups[i].members_count++;
//                     }
//                     break;
//                 }
//             }

//             //atualiza tópico GROUPS
//             char membersStr[256] = "";
//             for (int i = 0; i < groups_count; i++) {
//                 if (strcmp(groups[i].name, groupName) == 0) {
//                     for (int j = 0; j < groups[i].members_count; j++) {
//                         strcat(membersStr, groups[i].members[j]);
//                         if (j < groups[i].members_count - 1) strcat(membersStr, ",");
//                     }
//                     break;
//                 }
//             }
//             char updatePayload[512];
//             snprintf(updatePayload, sizeof(updatePayload), "GROUP:%s;LEADER:%s;MEMBERS:%s", groupName, username, membersStr);
//             publisherStatus(username, "GROUPS", updatePayload);

//             //resposta ao solicitante
//             char reply[128];
//             snprintf(reply, sizeof(reply), "JOIN_ACCEPT;GROUP:%s;USER:%s", groupName, username);
//             char requester_topic[80];
//             snprintf(requester_topic, sizeof(requester_topic), "%s_Control", requester);
//             publisherStatus(username, requester_topic, reply);

//             printf("Solicitação aceita e grupo atualizado.\n");
//         } else {
//             char reply[128];
//             snprintf(reply, sizeof(reply), "JOIN_REJECT;GROUP:%s;USER:%s", groupName, username);
//             publisherStatus(username, requester, reply);
//             printf("Solicitação rejeitada.\n");
//         }
//     }
// }

// // processa respostas de solicitação
// void processGroupResponse(const char* payload, const char* username) {
//     char action[32], groupName[64], responder[64];
//     if (sscanf(payload, "%31[^;];GROUP:%63[^;];USER:%63s", action, groupName, responder) != 3)
//         return;

//     if (strcmp(action, "JOIN_ACCEPT") == 0) {
//         printf("Sua solicitação para entrar no grupo '%s' foi aceita por '%s'\n", groupName, responder);
//     } else if (strcmp(action, "JOIN_REJECT") == 0) {
//         printf("Sua solicitação para entrar no grupo '%s' foi rejeitada por '%s'\n", groupName, responder);
//     }
// }

// void checkGroupRequests(const char* username) {
//     LinkedList temp_list;
//     listInit(&temp_list);

//     SubscribeArgs controlArgs;
//     strncpy(controlArgs.username, username, 64);
//     snprintf(controlArgs.topic, 64, "%s_Control", username);
//     controlArgs.message_list = &temp_list;

//     //chama subscriber apenas uma vez para ler mensagens
//     subscriberUsers(controlArgs.username, controlArgs.topic, controlArgs.message_list);

//     //processa cada mensagem recebida
//     Node* node = temp_list.head;
//     while (node) {
//         processGroupRequest(node->message, username);
//         processGroupResponse(node->message, username);
//         node = node->next;
//     }

//     //limpa a lista temporária
//     Node* n = temp_list.head;
//     while (n) {
//         Node* tmp = n;
//         n = n->next;
//         free(tmp); // não precisa free(tmp->message) porque é array interno
//     }
//     temp_list.head = NULL;
// }

// void startConversation() {
//     printf("Funcionalidade de conversa ainda não implementada.\n");
// }

// void showRequestHistory() {
//     printf("Histórico.\n");
// }

// void listClear(LinkedList* list) {
//     Node* current = list->head;
//     while (current) {
//         Node* tmp = current;
//         current = current->next;
//         free(tmp); // se message for alocado dinamicamente, faça free(tmp->message);
//     }
//     list->head = NULL;
// }

// void* subscriberControlThread(void* arg) { //nao funciona, era pra ficar verificando solicitações
//     SubscribeArgs* args = (SubscribeArgs*)arg;

//     while (online) {
//         Node* node = args->message_list->head;
//         while (node) {
//             processGroupRequest(node->message, args->username);
//             processGroupResponse(node->message, args->username);
//             node = node->next;
//         }

//         listClear(args->message_list); // limpa mensagens já processadas

//         #if defined(_WIN32)
//             Sleep(500);
//         #else
//             usleep(500000L); // 0,5s
//         #endif
//     }

//     return NULL;
// }

// Main Function

int main()
{
    // ----- Program Startup -----

    // Welcome & Username Definition

    char username[64];
    printf("\nChatMQTT\n\nDigite Seu Nome De Usuário (Máximo 63 Caractéres): ");
    scanf("%63s", username); // Limit Username Input To 63 Characters + '\0'

    if (strlen(username) == 0) {
        printf("Nome de usuário inválido.\n");
        return 1;
    }

    printf("\nBem Vindo, %s!\n\n", username);

    // Threads Parameters

    pthread_t threads[0]; // Threads Handler
    // Total Threads Number Is Based On The Maximum Possible Concurrent Threads:
    // Messages Queue (WIP)
    int threads_running = 0; // Threads Counter

    // Queues Initialization

    LinkedList status_list; // Status List
    listInit(&status_list);

    // LinkedList group_list; // Groups List
    // listInit(&group_list);

    // Send Online Status (USERS)

    setStatus(username, "Online");

    // Group groups[10];
    // int groups_count = 0;
    // char groupName[64];
    

    // #if defined(_WIN32)
	// 		Sleep(2500);
	// 	#else
	// 		usleep(2500000L);
	// 	#endif

    // SubscribeArgs controlArgs;
    // strncpy(controlArgs.username, username, 64);
    // snprintf(controlArgs.topic, 64, "%s_Control", username); // X_control
    // controlArgs.message_list = &group_list;

    // SubscribeArgs groupsArgs;
    // strncpy(groupsArgs.username, username, 64);
    // strncpy(groupsArgs.topic, "GROUPS", 64);
    // groupsArgs.message_list = &group_list;

    // pthread_t subThreads[2];
    // pthread_create(&subThreads[0], NULL, subscriberControlThread, &controlArgs);
    // pthread_create(&subThreads[1], NULL, subscriberControlThread, &groupsArgs);

    // Startup Safety Delay

    #if defined(_WIN32)
        Sleep(DELAY_5_SEC_MS); 
    #else
        usleep(DELAY_5_SEC_US);
    #endif

    // ----- Program Menu -----

    char menu_op1;
    char menu_op2;
    char menu_op3;
    
    while (1) { // Main Loop (Menu)

        // Menu Display

        printf("\n");

        printf("Menu:\n"
               "1. Listar Usuários\n"
               "2. Listar Grupos\n"
               "3. Conversar\n"
               "4. Criar Grupo\n"
               "5. Solicitações\n"
               "6. Sair\n\n");
        printf("> ");
        scanf(" %c", &menu_op1);

        // Menu Options

        if (menu_op1 == '1') // Listar Usuários
        { 
            getUsers(username, &status_list);
        }
        else if (menu_op1 == '2') // Listar Grupos
        {
            printf("WIP\n");
            // listGroups();
        }
        else if (menu_op1 == '3') // Conversar
        {
            printf("Conversar Com:\n"
                   "1. Amigos\n"
                   "2. Grupos\n");
            printf("> ");
            scanf("%c", &menu_op2);

            if (menu_op2 == '1') // Amigos
            {
                printf("WIP\n");
            }
            else if (menu_op2 == '2') // Grupos
            {
                printf("WIP\n");
            }
            //startConversation();
        }
        else if (menu_op1 == '4') // Criar Grupo
        {
            printf("Nome do Grupo: ");
            char groupName[64];
            printf("> ");
            scanf("%63s", groupName);
            printf("WIP\n");
            // createGroup(groupName, username);
        }
        else if (menu_op1 == '5') // Solicitações
        {
            printf("1. Minhas Solicitações De Entrada Em Grupos\n"
                   "2. Enviar Solicitações De Entrada Em Grupos\n"
                   "3. Minhas Solicitações De Conversa\n"
                   "4. Enviar Solicitações De Conversa\n");
            printf("> ");
            scanf("%c", &menu_op3);

            if (menu_op3 == '1') // Ver (Grupos)
            {
                printf("WIP\n");
                //checkGroupRequests(username);
            }
            else if (menu_op3 == '2') // Enviar (Grupos)
            {
                printf("Nome do Grupo: ");
                char groupName[64];
                printf("> ");
                scanf("%63s", groupName);

                printf("Nome do líder do grupo: ");
                char leader[64];
                printf("> ");
                scanf("%63s", leader);

                printf("WIP\n");
                //requestJoinGroup(groupName, username, leader);
            }
            else if (menu_op3 == '3') // Ver (Conversa)
            {
                printf("WIP\n");
            } 
            else if (menu_op3 == '4') // Enviar (Conversa)
            {
                printf("WIP\n");
            }
            else
            {
                printf("Opção Inválida!\n");
            }
        }
        else if(menu_op1 == '6') // Sair
        {
            printf("\nSaindo...\n");
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

    // ----- Program Shutdown -----

    // Shutdown Safety Delay

    #if defined(_WIN32)
			Sleep(DELAY_5_SEC_MS);
		#else
			usleep(DELAY_5_SEC_US);
		#endif

    online = 0;

    // Wait For Threads Completion

    for (int i = 0; i < threads_running; i++)
    {
        pthread_join(threads[i], NULL);
    }

    // Send Offline Status (USERS)

    setStatus(username, "Offline");

    // End

    printf("\n");

    printf("Até Mais, %s!\n\n", username);

    return 0;
}