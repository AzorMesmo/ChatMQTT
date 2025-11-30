// Compilation Command: "gcc main.c publisher.c subscriber.c agent.c messages.c -o main -lpaho-mqtt3as -pthread"
// Excecution Command: "./main"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include "constants.h"
#include "publisher.h"
#include "subscriber.h"
#include "messages.h"
#include "agent.h"

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
    char topic[1024];
    char payload[512];
    volatile int* online;
} PublishArgs;

typedef struct // Subscriber Arguments
{
    char username[64];
    char topic[1024];
	LinkedList* message_list;
    volatile int* online;
} SubscribeArgs;

typedef struct // Agent Arguments
{
    char username[64];
    char topic[1024];
    LinkedList* message_list;
    volatile int* online;
} AgentArgs;

typedef struct // Conversation (Subscriber) Arguments
{
    char username[64];
    char topic[1024];
    LinkedList* message_list;
    volatile int* chatting;
} ConversationArgs;

// Default Functions

// Set User Status (Online / Offline)
void setStatus(const char* username, const char* status)
{
    // [USERNAME]:[STATUS]
    char topic[1024];
    char payload[512];

    snprintf(topic, sizeof(topic), "USERS/%s", username);
    snprintf(payload, sizeof(payload), "%s:%s", username, status);

    publisher(username, topic, payload, 1);
}

// Get Users Status (Online / Offline)
void getUsers(const char* username, LinkedList* status_list, int print_status)
{ 
    // print_status: 1 = Print, 0 = Don't Print
    listClear(status_list);
    subscriberRetained(username, "USERS/+", status_list);
    if (print_status)
    {
        listPrintStatus(status_list);
    }
}

// Get Groups (Name / Leader / Members)
void getGroups(const char* username, LinkedList* groups_list, int print_groups)
{
    // print_groups: 1 = Print, 0 = Don't Print
    listClear(groups_list);
    subscriberRetained(username, "GROUPS/+", groups_list);
    if (print_groups)
    {
        listPrintGroups(groups_list);
    }
}

// Get Requests
void getRequests(const char* username, LinkedList* requests_list, int print_requests)
{
    // print_requests: 1 = Print, 0 = Don't Print
    listClear(requests_list);
    char temp[128];
    snprintf(temp, sizeof(temp), "%s_Control/REQUESTS/+", username);
    subscriberRetained(username, temp, requests_list);
    if (print_requests)
    {
        listPrintRequests(requests_list);
    }
}

// Get History
void getHistory(const char* username, LinkedList* history_list, int print_history)
{
    // print_history: 1 = Print, 0 = Don't Print
    listClear(history_list);
    char temp[128];
    snprintf(temp, sizeof(temp), "%s_Control/HISTORY/+", username);
    subscriberRetained(username, temp, history_list);
    if (print_history)
    {
        listPrintHistory(history_list, username);
    }
}

// Get Chats (Name)
int getChats(const char* username, LinkedList* history_list, int print_chats)
{
    // print_chats: 1 = Print, 0 = Don't Print
    listClear(history_list);
    char temp[128];
    snprintf(temp, sizeof(temp), "%s_Control/HISTORY/+", username);
    subscriberRetained(username, temp, history_list);
    if (print_chats)
    {
        listPrintChats(history_list, username);
    }
}

// Create Group (Name / Leader / Members)
void setGroup(const char* groupname, const char* username)
{
    // [GROUP_NAME]:[LEADER]:[MEMBER1;MEMBER2;...]
    char topic[1024];
    char payload[512];
    char link[256];

    // Publish On Groups
    snprintf(topic, sizeof(topic), "GROUPS/%s", groupname);
    snprintf(payload, sizeof(payload), "%s:%s:%s;", groupname, username, username); // Leader Is The First Member

    publisher(username, topic, payload, 1);

    // Publish On History
    char timestamp[100];
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H-%M-%S", t);

    snprintf(link, sizeof(link), "%s|%s", groupname, timestamp);
    snprintf(payload, sizeof(payload), "GROUP_CREATED:%s;%s", groupname, link);
    snprintf(topic, sizeof(topic), "%s_Control/HISTORY/%s", username, payload);

    publisher(username, topic, payload, 1);

    // Subscribe On Conversation Topic
    snprintf(topic, sizeof(topic), "CHATS/%s", link);

    subscriberDirty(username, topic, NULL);
}

// Request User Coversation
void requestUser(const char* username, const char* user)
{
    char topic[512]; // Topic = [TARGET_USER]_Control
    char my_topic[512]; // Topic = [USERNAME]_Control/HISTORY/[BODY]
    char request[256]; // USER_REQUEST:[USERNAME]
    char history[256]; // USER_REQUEST_SENT:[TARGET_USER]

    // Request
    snprintf(topic, sizeof(topic), "%s_Control", user);
    snprintf(request, sizeof(request), "USER_REQUEST:%s", username);

    publisher(username, topic, request, 0);

    // History
    snprintf(history, sizeof(history), "USER_REQUEST_SENT:%s", user);
    snprintf(my_topic, sizeof(my_topic), "%s_Control/HISTORY/%s", username, history);

    publisher(username, my_topic, history, 1);
}

// Request User Coversation
void requestGroup(const char* username, const char* leader, const char* group)
{
    char topic[512]; // Topic = [TARGET_LEADER]_Control
    char my_topic[512]; // Topic = [USERNAME]_Control/HISTORY/[BODY]
    char request[256]; // GROUP_REQUEST:[GROUPNAME];[USERNAME]
    char history[256]; // GROUP_REQUEST_SENT:[GROUPNAME];[LEADER]

    // Request
    snprintf(topic, sizeof(topic), "%s_Control", leader);
    snprintf(request, sizeof(request), "GROUP_REQUEST:%s;%s", group, username);
    
    publisher(username, topic, request, 0);

    // History
    snprintf(history, sizeof(history), "GROUP_REQUEST_SENT:%s;%s", group, leader);
    snprintf(my_topic, sizeof(my_topic), "%s_Control/HISTORY/%s", username, history);

    publisher(username, my_topic, history, 1);
}

// Respond Group Coversation
void respondUser(const char* username, const char* user, const char* link, const char* my_response)
{
    char topic[512]; // Topic = [USER]_Control
    char my_topic[512]; // Topic = [USERNAME]_Control/HISTORY/[BODY]
    char response[256]; // USER_ACCEPTED:[USERNAME];[TOPIC] | USER_REJECTED:[USERNAME]
    char history[256]; // USER_ACCEPTED:[USERNAME];[TOPIC]  | USER_REJECTED:[USERNAME]

    if (strcmp(my_response, "ACEITAR") == 0) // ACCEPTED
    {
        // Request
        snprintf(topic, sizeof(topic), "%s_Control", user);
        snprintf(response, sizeof(response), "USER_ACCEPTED:%s;%s", username, link);
        
        publisher(username, topic, response, 0);

        // History
        snprintf(history, sizeof(history), "USER_ACCEPTED:%s;%s", user, link);
        snprintf(my_topic, sizeof(my_topic), "%s_Control/HISTORY/%s", username, history);

        publisher(username, my_topic, history, 1);
    }
    else // REJECTED
    {
        // Request
        snprintf(topic, sizeof(topic), "%s_Control", user);
        snprintf(response, sizeof(response), "USER_REJECTED:%s", username);
        
        publisher(username, topic, response, 0);

        // History
        snprintf(history, sizeof(history), "USER_REJECTED:%s", user);
        snprintf(my_topic, sizeof(my_topic), "%s_Control/HISTORY/%s", username, history);

        publisher(username, my_topic, history, 1);
    }
}

// Respond Group Coversation
void respondGroup(const char* username, const char* group, const char* user, const char* link, const LinkedList* groups_list, const char* my_response)
{
    char topic[512]; // Topic = [USER]_Control
    char my_topic[512]; // Topic = [USERNAME]_Control/HISTORY/[BODY]
    char response[256]; // GROUP_ACCEPTED:[GROUPNAME];[USER];[TOPIC] | GROUP_REJECTED:[GROUPNAME];[USER]
    char history[256]; // GROUP_ACCEPTED:[GROUPNAME];[USER]          | GROUP_REJECTED:[GROUPNAME];[USER]
    char *group_info; // Message = [GROUPNAME]:[USERNAME]:[MEMBER];
    char group_info_new[2048]; // Message = [GROUPNAME]:[USERNAME]:[MEMBER];[MEMBER];
    char group_topic[512]; // Topic = GROUPS/[GROUPNAME]

    if (strcmp(my_response, "ACEITAR") == 0) // ACCEPTED
    {
        // Update "GROUPS/"
        group_info = listGetGroup(groups_list, group, username);
        snprintf(group_topic, sizeof(group_topic), "GROUPS/%s", group);
        snprintf(group_info_new, sizeof(group_info_new), "%s%s;", group_info, user);

        publisher(username, group_topic, group_info_new, 1);

        // Request
        snprintf(topic, sizeof(topic), "%s_Control", user);
        snprintf(response, sizeof(response), "GROUP_ACCEPTED:%s;%s;%s", group, username, link);
        
        publisher(username, topic, response, 0);

        // History
        snprintf(history, sizeof(history), "GROUP_ACCEPTED:%s;%s", group, user);
        snprintf(my_topic, sizeof(my_topic), "%s_Control/HISTORY/%s", username, history);

        publisher(username, my_topic, history, 1);
    }
    else // REJECTED
    {
        // Request
        snprintf(topic, sizeof(topic), "%s_Control", user);
        snprintf(response, sizeof(response), "GROUP_REJECTED:%s;%s", group, username);
        
        publisher(username, topic, response, 0);

        // History
        snprintf(history, sizeof(history), "GROUP_REJECTED:%s;%s", group, user);
        snprintf(my_topic, sizeof(my_topic), "%s_Control/HISTORY/%s", username, history);

        publisher(username, my_topic, history, 1);
    }
}

// Create Conversation Topic By Sending "WATING_USER"
void createConversation(const char* username, const char* link)
{
    char topic[1024];

    snprintf(topic, sizeof(topic), "CHATS/%s", link);

    subscriberDirty(username, topic, NULL);
    publisherDirty(username, topic, "WAITING_USER", 1);
}

// Check If Conversation Topic Is Ready
int checkConversation(const char* username, const char* link, LinkedList* message_list)
{
    char topic[1024];
    char pseudouser[128]; // To Avoid Receiving Messages In The Wrong Time
    snprintf(topic, sizeof(topic), "CHATS/%s", link);
    snprintf(pseudouser, sizeof(pseudouser), "%s;", username);
    listClear(message_list);
    subscriberRetained(pseudouser, topic, message_list);
    if (listSearch(message_list, "WAITING_USER") != 0)
    {
        return 0;
    }
    return 1;
}

// Monitor Control Topic ([USER]_Control) > Used With Threads
void monitorControl(const char* username, LinkedList* control_list, volatile int* online)
{
    char control_username[72];
    snprintf(control_username, sizeof(control_username), "%s_Control", username);

    listClear(control_list);

    agentControl(control_username, control_list, online);
}

// Realtime Conversation Confirmation > Used With Threads
void confirmationControl(const char* username, LinkedList* control_list, volatile int* online)
{
    while (*online)
    {
        char* element = listPopLast(control_list);
        if (element) // If There's A New Element
        {
            subscriberDirty(username, element, NULL);
            free(element);
        }

        #if defined(_WIN32)
            Sleep(DELAY_100_MS_MS);
        #else
            usleep(DELAY_100_MS_US);
        #endif
    }
}

// Process Request
void processRequest(const char* username, const char* request)
{
    char topic[512]; // Topic = [USERNAME]_Control/REQUESTS/[REQUEST]
    snprintf(topic, sizeof(topic), "%s_Control/REQUESTS/%s", username, request);
    publisher(username, topic, "", 1); // Publish Empty Payload To Remove Retained Message
}

// Thread Function Wrappers

// monitorControl Thread Wrapper
void* monitorControlThread(void* arg)
{
    AgentArgs* args = (AgentArgs*)arg;
    monitorControl(args->username, args->message_list, args->online);
    free(args);  // Free Arguments Structure
    return NULL;
}

// confirmationControl Thread Wrapper
void* confirmationControlThread(void* arg)
{
    SubscribeArgs* args = (SubscribeArgs*)arg;
    confirmationControl(args->username, args->message_list, args->online);
    free(args);  // Free Arguments Structure
    return NULL;
}

// subscriberConversation (startConversation) Thread Wrapper
void* subscriberConversationThread(void* arg)
{
    ConversationArgs* args = (ConversationArgs*)arg;
    subscriberConversation(args->username, args->topic, args->message_list, args->chatting);
    free(args);  // Free Arguments Structure
    return NULL;
}

// Main Function

int main()
{
    // ----- Program Startup -----

    // Welcome & Username Definition

    printf("\nChatMQTT\n");

    char username[64];
    int username_undefined = 1;

    while (username_undefined) // Validity Checker
    {
        printf("\nDigite Seu Nome De Usuário (Máximo 63 Caractéres): ");
        scanf("%63s", username); // Limit Username Input To 63 Characters + '\0'

        if (strlen(username) == 0 || strspn(username, " \t\n\r") == strlen(username)) { // No Username | Only Whitespaces
            printf("Nome De Usuário Não Pode Estar Vazio!\n");
            continue;
        }

        if (strpbrk(username, ":/+#;") != NULL) { // Username Contains Invalid Characters (':', '/', '+', '#', ';', '|')
            printf("Nome De Usuário Contém Caractéres Inválidos! (':', '/', '+', '#', ';', '|')\n");
            continue;
        }

        username_undefined = 0;
    }

    printf("\nBem Vindo, %s!\n", username);

    if(LOG_ENABLED)
        printf("\n");

    // Threads Parameters

    pthread_t threads[2]; // Threads Handler
    // Total Threads Number Is Based On The Maximum Possible Concurrent Threads:
    // - Control Topic Agent (Publisher/Subscriber)
    // - User Realtime Conversation Confirmation
    int threads_running = 0; // Threads Counter

    // Queues Initialization

    LinkedList status_list; // Status (Users) List
    listInit(&status_list);

    LinkedList groups_list; // Groups List
    listInit(&groups_list);

    LinkedList control_list; // Control List
    listInit(&control_list);

    LinkedList requests_list; // Requests List
    listInit(&requests_list);

    LinkedList history_list; // History List
    listInit(&history_list);

    LinkedList messages_list; // Messages List
    listInit(&messages_list);

    // Control Topic Thread Inicialization

    AgentArgs* control_args = malloc(sizeof(AgentArgs));
    strncpy(control_args->username, username, sizeof(control_args->username) - 1);
    control_args->message_list = &control_list;
    control_args->online = &online;

    if (pthread_create(&threads[threads_running], NULL, monitorControlThread, control_args) != 0) {
        printf("Erro Ao Iniciar O Programa! (Control Topic Thread Inicialization Failed)\n");
        free(control_args);
        return EXIT_FAILURE;
    }
    threads_running++;

    SubscribeArgs* subscribe_args = malloc(sizeof(SubscribeArgs));
    strncpy(subscribe_args->username, username, sizeof(subscribe_args->username) - 1);
    subscribe_args->message_list = &control_list;
    subscribe_args->online = &online;

    if (pthread_create(&threads[threads_running], NULL, confirmationControlThread, subscribe_args) != 0) {
        printf("Erro Ao Iniciar O Programa! (Confirmation Thread Inicialization Failed)\n");
        free(subscribe_args);
        return EXIT_FAILURE;
    }
    threads_running++;

    // Send Online Status (USERS)

    setStatus(username, "Online");

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
               "1. Listar Usuários\n" // OK
               "2. Listar Grupos\n"   // OK
               "3. Conversar\n"
               "4. Criar Grupo\n"     // OK
               "5. Solicitações\n"
               "6. Sair\n\n");        // OK
        printf("> ");
        scanf(" %c", &menu_op1);

        // Menu Options

        // 1 - Listar Usuários
        if (menu_op1 == '1')
        { 
            printf("\nBuscando Usuários...\n");

            if(LOG_ENABLED)
                printf("\n");

            getUsers(username, &status_list, 1);

            if (status_list.head == NULL)
            {
                printf("\nNenhum Usuário Encontrado.\n");
            }
        }

        // 2 - Listar Grupos
        else if (menu_op1 == '2')
        {
            printf("\nBuscando Grupos...\n");

            if(LOG_ENABLED)
                printf("\n");

            getGroups(username, &groups_list, 1);
            
            if (groups_list.head == NULL)
            {
                printf("\nNenhum Grupo Encontrado.\n");
            }
        }

        // 3 - Conversar
        else if (menu_op1 == '3')
        {
            printf("\nBuscando Conversas...\n\n");

            if(LOG_ENABLED)
                printf("\n");

            getChats(username, &history_list, 1);

            if (listSearchChat(&history_list) != 0)
            {
                char user_request = 'N';
                printf("Deseja Entrar Em Uma Conversa? (S/N)\n\n");
                printf("> ");
                scanf(" %c", &user_request);

                if (user_request == 'S' || user_request == 's')
                {

                    int target_conversation_undefined = 1;
                    char *type = NULL;
                    char *target = NULL;

                    while (target_conversation_undefined) // Validity Checker
                    {
                        char user_response[256];
                        printf("\nDigite:\n"
                            "- Usuário > \"U:[NOME DE USUÁRIO]\"\n"
                            "- Grupo   > \"G:[NOME DO GRUPO]\"\n"
                            "- Sair    > \":\"\n\n");
                        printf("> ");
                        scanf("%255s", user_response);

                        if (strlen(user_response) == 0 || strspn(user_response, " \t\n\r") == strlen(user_response)) // No Target / Only Whitespaces
                        { 
                            printf("Comando Não Pode Estar Vazio!\n");
                            continue;
                        }
                        if (strchr(user_response, ':') == NULL)
                        {
                            printf("Estrutura Do Comando Inválida!\n");
                            continue;
                        }
                        if (strcmp(user_response, ":") == 0) // EXIT
                        {
                            break;
                        }

                        type = strtok(user_response, ":");

                        if (!type || (strcmp(type, "U") != 0 && strcmp(type, "G") != 0))
                        {
                            printf("Comando Inválido!\n");
                            continue;
                        }

                        target = strtok(NULL, ":");

                        if (!target)
                        {
                            printf("Usuário Inválido!\n");
                            continue;
                        }
                        if (listSearchConversation(&history_list, target) == 0) 
                        { 
                            printf("Você Não Possui Essa Conversa!\n");
                            continue;
                        }

                        target_conversation_undefined = 0;
                    }
                    
                    if (!target_conversation_undefined)
                    {
                        char* link = listGetTopic(&history_list, target);

                        // If User, Check If Ready
                        if (strcmp(type, "U") == 0)
                        { 
                            if (checkConversation(username, link, &messages_list) == 0)
                            {
                                printf("Aguardando Outro Usuário...\n");
                                continue;
                            }
                        }

                        volatile int chatting = 1;

                        char topic[1024];
                        snprintf(topic, sizeof(topic), "CHATS/%s", link);
                        free(link);

                        pthread_t chat_thread[1];

                        ConversationArgs* conversation_args = malloc(sizeof(ConversationArgs));
                        strncpy(conversation_args->username, username, sizeof(conversation_args->username) - 1);
                        strncpy(conversation_args->topic, topic, sizeof(conversation_args->topic) - 1);
                        conversation_args->message_list = &messages_list;
                        conversation_args->chatting = &chatting;

                        if (pthread_create(&chat_thread[0], NULL, subscriberConversationThread, conversation_args) != 0)
                        {
                            printf("Erro Ao Iniciar Conversa!\n");
                            free(conversation_args);
                            return EXIT_FAILURE;
                        }

                        char pseudousername[128];
                        snprintf(pseudousername, sizeof(pseudousername), "%s;", username);

                        printf("\nIniciando Conversa...\n\n"
                            "- Escreva Normalmente Para Enviar Mensagens\n"
                            "- Digite \";\" Para Procurar Novas Mensagens (Não Atualiza Automaticamente)\n"
                            "- Digite \":\" Para Sair\n\n");
                        char message[16256];
                        char formatted_message[16384];

                        while (chatting)
                        {
                            fflush(stdout);
                            if (fgets(message, sizeof(message), stdin) == NULL) {
                                chatting = 0;
                                break;
                            }
                            // Remove trailing newline(s)
                            message[strcspn(message, "\r\n")] = '\0';

                            if (strlen(message) == 0 || strspn(message, " \t\n\r") == strlen(message)) // No Groupname / Only Whitespaces
                            { 
                                continue;
                            }

                            if (strcmp(message, ":") == 0) // EXIT
                            {
                                break;
                            }

                            if (strcmp(message, ";") == 0) // Uptade Messages
                            {
                                listPopPrintAll(&messages_list);
                                continue;
                            }

                            
                            snprintf(formatted_message, sizeof(formatted_message), "%s: %s", username, message);
                            publisherDirty(pseudousername, topic, formatted_message, 0);
                        }

                        chatting = 0;
                        pthread_join(chat_thread[0], NULL);
                    }
                }
                else
                {
                    continue; 
                }
            }
            else
            {
                printf("\nNenhuma Conversa Encontrada.\n");
                continue;
            }
        }

        // 4 - Criar Grupo
        else if (menu_op1 == '4')
        {
            getGroups(username, &groups_list, 0);

            char groupname[64];
            int groupname_undefined = 1;

            while (groupname_undefined) // Validity Checker
            {
                printf("\nDigite O Nome Do Grupo (Máximo 63 Caractéres): ");
                scanf("%63s", groupname); // Limit Groupname Input To 63 Characters + '\0'

                if (strlen(groupname) == 0 || strspn(groupname, " \t\n\r") == strlen(groupname)) { // No Groupname / Only Whitespaces
                    printf("Nome Do Grupo Não Pode Estar Vazio!\n");
                    continue;
                }

                if (strpbrk(groupname, ":/+#;|") != NULL) { // Groupname Contains Invalid Characters (':', '/', '+', '#', ';', '|')
                    printf("Nome Do Grupo Contém Caractéres Inválidos! (':', '/', '+', '#', ';', '|')\n");
                    continue;
                }

                if (listSearchFirstParameter(&groups_list, groupname) != 0) { // Groupname Already Exists
                    printf("Grupo Já Existe, Escolha Outro Nome!\n");
                    continue;
                }

                groupname_undefined = 0;
            }

            printf("\n");

            setGroup(groupname, username);

            printf("Grupo Criado Com Sucesso!\n");
        }

        // 5 - Solicitações
        else if (menu_op1 == '5')
        {
            printf("\n1. Ver Solicitações Pendentes\n"
                   "2. Ver Histórico De Eventos\n"
                   "3. Solicitar Conversa Com Usuário\n"
                   "4. Solicitar Conversa Com Grupo\n\n");
            printf("> ");
            scanf(" %c", &menu_op3);

            // 5.1 - Ver Solicitações Pendentes
            if (menu_op3 == '1')
            {
                getRequests(username, &requests_list, 1);

                if (requests_list.head != NULL)
                {  
                    char user_request_accept = 'N';
                    printf("\nDeseja Responder À Uma Solicitação? (S/N)\n\n");
                    printf("> ");
                    scanf(" %c", &user_request_accept);

                    if (user_request_accept == 'S' || user_request_accept == 's')
                    {
                        getUsers(username, &status_list, 0);
                        getGroups(username, &groups_list, 0);
                        getHistory(username, &history_list, 0);

                        char user_response[256];
                        printf("\nDigite:\n"
                            "- Usuário > \"[NOME DE USUÁRIO]:[RESPOSTA]\"\n"
                            "- Grupo   > \"[NOME DO GRUPO];[NOME DE USUÁRIO]:[RESPOSTA]\"\n"
                            "- Sair    > \":\"\n"
                            "[RESPOSTA] = \"ACEITAR\" | \"REJEITAR\"\n\n");
                        printf("> ");
                        scanf("%255s", user_response);

                        if (strcmp(user_response, ":") == 0) // EXIT
                        {
                            continue;
                        }
                        else if (strchr(user_response, ';') != NULL) // GROUP
                        {
                            char* group = strtok(user_response, ";"); // Groupname
                            char* user = strtok(NULL, ":"); // Username
                            char* response = strtok(NULL, ":"); // Response

                            if (!group || !user || !response) // ":" Or ";" Not Found
                            {
                                printf("\nOpção Inválida!\n");
                                continue;
                            }

                            toUppercase(response);

                            if (strcmp(response, "ACEITAR") != 0 && strcmp(response, "REJEITAR") != 0)
                            {
                                printf("\nResposta Inválida!\n");
                                continue;
                            }

                            char formatted_group[1024];
                            snprintf(formatted_group, sizeof(formatted_group), "GROUP_REQUEST:%s;%s", group, user);

                            if (listSearch(&requests_list, formatted_group) == 0)
                            {
                                printf("\nUsuário Ou Grupo Inválido!\n");
                                continue;
                            }

                            char* topic = listGetTopic(&history_list, group);

                            respondGroup(username, group, user, topic, &groups_list, response);
                            free(topic);

                            processRequest(username, formatted_group);
                        }
                        else // USER
                        {
                            char* user = strtok(user_response, ":"); // Username
                            char* response = strtok(NULL, ":"); // Response

                            if (!user || !response) // ":" Not Found
                            {
                                printf("\nOpção Inválida!\n");
                                continue;
                            }

                            toUppercase(response);

                            if (strcmp(response, "ACEITAR") != 0 && strcmp(response, "REJEITAR") != 0)
                            {
                                printf("\nResposta Inválida!\n");
                                continue;
                            }

                            char formatted_user[300];
                            snprintf(formatted_user, sizeof(formatted_user), "USER_REQUEST:%s", user);

                            if (listSearch(&requests_list, formatted_user) == 0)
                            {
                                printf("\nUsuário Inválido!\n");
                                continue;
                            }
                            
                            char timestamp[100];
                            time_t now = time(NULL);
                            struct tm *t = localtime(&now);
                            strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H-%M-%S", t);

                            char topic[1024];
                            snprintf(topic, sizeof(topic), "%s_%s|%s", username, user, timestamp);

                            if (strcmp(response, "ACEITAR") == 0)
                            {
                                createConversation(username, topic);
                            }

                            respondUser(username, user, topic, response);

                            processRequest(username, formatted_user);
                        }      
                    }
                    else
                    {
                        continue; 
                    }
                }
                else
                {
                    printf("\nVocê Não Possui Solicitações Pendentes.\n"); 
                }
            }

            // 5.2 - Ver Histórico De Eventos
            else if (menu_op3 == '2')
            {
                getHistory(username, &history_list, 1);

                if (history_list.head == NULL)
                {
                    printf("\nVocê Não Possui Nenhum Evento No Histórico.\n");
                }
            }
            
            // 5.3 - Solicitar (Usuário)
            else if (menu_op3 == '3')
            {
                printf("\nBuscando Usuários...\n");

                if(LOG_ENABLED)
                    printf("\n");

                getUsers(username, &status_list, 1);
                getHistory(username, &history_list, 0);

                if (status_list.head == NULL)
                {
                    printf("\nNenhum Usuário Encontrado.\n");
                }

                char user_request = 'N';
                printf("\nDeseja Solicitar Uma Conversa? (S/N)\n\n");
                printf("> ");
                scanf(" %c", &user_request);

                if (user_request == 'S' || user_request == 's')
                {
                    char target_user[64];
                    int target_user_undefined = 1;

                    while (target_user_undefined) // Validity Checker
                    {
                        printf("\nDigite O Nome Do Usuário Escolhido, Ou \":\" Para Cancelar: ");
                        scanf("%63s", target_user);

                        if (strcmp(target_user, ":") == 0) // EXIT
                        {
                            break;
                        }

                        if (strlen(target_user) == 0 || strspn(target_user, " \t\n\r") == strlen(target_user)) // No Target User / Only Whitespaces
                        { 
                            printf("O Nome Do Usuário Não Pode Estar Vazio!\n");
                            continue;
                        }
 
                        if (listSearchFirstParameter(&status_list, target_user) == 0) // Target User Not Found In Online List
                        { 
                            printf("O Nome Do Usuário É Inválido!\n");
                            continue;

                        }

                        if (listSearchConversation(&history_list, target_user) != 0) // Conversation Already Exists
                        { 
                            printf("Você Já Possui Uma Conversa Com Este Usuário!\n");
                            continue;
                        }

                        target_user_undefined = 0;
                    }
                    
                    if (!target_user_undefined)
                    {
                        requestUser(username, target_user);
                    }
                }
                else
                {
                    continue; 
                }
            }

            // 5.4 - Solicitar (Grupo)
            else if (menu_op3 == '4')
            {
                printf("\nBuscando Grupos...\n");

                if(LOG_ENABLED)
                    printf("\n");

                getGroups(username, &groups_list, 1);
                getHistory(username, &history_list, 0);
                
                if (groups_list.head == NULL)
                {
                    printf("\nNenhum Grupo Encontrado.\n");
                }

                char group_request = 'N';
                printf("\nDeseja Solicitar A Entrada Em Um Grupo? (S/N)\n\n");
                printf("> ");
                scanf(" %c", &group_request);

                if (group_request == 'S' || group_request == 's')
                {
                    char target_group[64];
                    char* target_leader;
                    int target_group_undefined = 1;

                    while (target_group_undefined) // Validity Checker
                    {
                        printf("\nDigite O Nome Do Grupo Escolhido, Ou \":\" Para Cancelar: ");
                        scanf("%63s", target_group);

                        if (strcmp(target_group, ":") == 0) // EXIT
                        {
                            break;
                        }

                        if (strlen(target_group) == 0 || strspn(target_group, " \t\n\r") == strlen(target_group)) // No Target User / Only Whitespaces
                        { 
                            printf("O Nome Do Grupo Não Pode Estar Vazio!\n");
                            continue;
                        }

                        if (listSearchFirstParameter(&groups_list, target_group) == 0) // Target Group Not Found In List
                        { 
                            printf("O Nome Do Grupo É Inválido!\n");
                            continue;

                        }

                        if (listSearchConversation(&history_list, target_group) != 0) // Conversation Already Exists
                        { 
                            printf("Você Já Possui Uma Conversa Com Este Grupo!\n");
                            continue;
                        }

                        target_group_undefined = 0;
                    }
                    
                    if (!target_group_undefined)
                    {
                        printf("\n");

                        printf("Grupo Escolhido: %s\n", target_group);

                        target_leader = listGetGroupLeader(&groups_list, target_group);
                        printf("Líder: %s\n", target_leader);

                        requestGroup(username, target_leader, target_group);

                        free(target_leader);
                    }
                }
                else
                {
                    continue; 
                }
            }

            // 5.X - Opção Inválida
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

        // X - Opção Inválida
        else
        {
            printf("Opção Inválida!\n");
        }
    }

    printf("\n");

    // ----- Program Shutdown -----

    // Shutdown Safety Delay

    online = 0;

    #if defined(_WIN32)
			Sleep(DELAY_5_SEC_MS);
		#else
			usleep(DELAY_5_SEC_US);
		#endif

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