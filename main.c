// Compilation Command: "gcc main.c publisher.c subscriber.c agent.c messages.c -o main -lpaho-mqtt3as -pthread"
// Excecution Command: "./main"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
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
    char topic[128];
    char payload[1024];
} PublishArgs;

typedef struct // Subscriber Arguments
{
    char username[64];
    char topic[128];
	LinkedList* message_list;
} SubscribeArgs;

typedef struct // Agent Arguments
{
    char username[64];
    char topic[128];
    LinkedList* message_list;
    volatile int* online;
} AgentArgs;

// typedef struct // Groups
// { 
//     char name[64];
//     char leader[64];
//     char members[10][64]; // Exemplo: até 10 membros
//     int members_count;
// } Group;

// Default Functions

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

// Set User Status (Online / Offline)
void setStatus(const char* username, const char* status)
{
    // [USERNAME]:[STATUS]
    char topic[80];
    char payload[128];

    snprintf(topic, sizeof(topic), "USERS/%s", username);
    snprintf(payload, sizeof(payload), "%s:%s", username, status);

    publisher(username, topic, payload, 1);
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

// Create Group (Name / Leader / Members)
void setGroup(const char* groupname, const char* username)
{
    // [GROUP_NAME]:[LEADER]:[MEMBER1;MEMBER2;...]
    char topic[80];
    char payload[128];

    snprintf(topic, sizeof(topic), "GROUPS/%s", groupname);
    snprintf(payload, sizeof(payload), "%s:%s:%s;", groupname, username, username); // Leader Is The First Member

    publisher(username, topic, payload, 1);
}

// adiciona novo membro no grupo
void updateGroupMembers(const char* groupname, const char* leader, const char* new_member) {
    char topic[80];
    char payload[2048];
    char membros_buffer[1024] = "";

    // le grupos atuais 
    LinkedList groups_list;
    listInit(&groups_list);
    subscriberRetained(leader, "GROUPS/+", &groups_list);//retida

    // percorre grupos até encontrar o grupo alvo
    Node* node = groups_list.head;
    int group_found = 0;
    while (node != NULL) {
        if (strncmp(node->message, groupname, strlen(groupname)) == 0) {
            // formato do payload é: "grupo:lider:m1;m2;..."
            char* membros = strstr(node->message, ":");
            if (membros) {
                membros = strstr(membros + 1, ":"); // pula nome e líder
                if (membros) {
                    membros++; // pula ':'
                    strncpy(membros_buffer, membros, sizeof(membros_buffer) - 1);
                    membros_buffer[strcspn(membros_buffer, "\n")] = '\0'; // remove quebra de linha
                }
            }
            group_found = 1;
            break;
        }
        node = node->next;
    }

    // se o membro ainda não existe - adiciona
    if (strstr(membros_buffer, new_member) == NULL) {
        strncat(membros_buffer, new_member, sizeof(membros_buffer) - strlen(membros_buffer) - 1);
        strncat(membros_buffer, ";", sizeof(membros_buffer) - strlen(membros_buffer) - 1);
    }

    //  topico e payload atualizados
    snprintf(topic, sizeof(topic), "GROUPS/%s", groupname);
    snprintf(payload, sizeof(payload), "%s:%s:%s", groupname, leader, membros_buffer);

    // publica atualização como retained 
    publisher(leader, topic, payload, 1);

    listDestroy(&groups_list);
}

// Monitor Control Topic (User_Control) > Used With Threads
void monitorControl(const char* username, LinkedList* control_list, volatile int* online)
{
    char control_username[72];
    snprintf(control_username, sizeof(control_username), "%s_Control", username);

    listClear(control_list);

    agentControl(control_username, control_list, online);
}

// Get Requests
void getRequests(const char* username, LinkedList* control_list, int print_control)
{
    // print_control: 1 = Print, 0 = Don't Print
    listClear(control_list);
    char temp[128];
    snprintf(temp, sizeof(temp), "%s_Control/REQUESTS/+", username);
    subscriberRetained(username, temp, control_list);
    if (print_control)
    {
        listPrintRequests(control_list);
    }
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

    pthread_t threads[1]; // Threads Handler
    // Total Threads Number Is Based On The Maximum Possible Concurrent Threads:
    // - Control Topic Agent (Publisher/Subscriber)
    int threads_running = 0; // Threads Counter

    // Queues Initialization

    LinkedList status_list; // Status List
    listInit(&status_list);

    LinkedList groups_list; // Groups List
    listInit(&groups_list);

    LinkedList control_list; // Control List (Conversation/Group Requets)
    listInit(&control_list);

    LinkedList online_users_list; // Online Users List
    listInit(&online_users_list);

    LinkedList online_groups_list; // Groups With Online Leaders List
    listInit(&online_groups_list);

    LinkedList received_history; // historico de sol. recebidas
    listInit(&received_history);

    LinkedList sent_history; // hist. de sol. enviadas
    listInit(&sent_history);

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
        }

        // 2 - Listar Grupos
        else if (menu_op1 == '2')
        {
            printf("\nBuscando Grupos...\n");

            if(LOG_ENABLED)
                printf("\n");

            getGroups(username, &groups_list, 1);
        }

        // 3 - Conversar
        else if (menu_op1 == '3')
        {
            printf("\nConversar Com:\n"
                   "1. Usuário\n"
                   "2. Grupo\n\n");
            printf("> ");
            scanf(" %c", &menu_op2);

            // 3.1 - Amigos
            if (menu_op2 == '1') 
            {
                printf("WIP\n");
            }
            // 3.2 - Grupos
            else if (menu_op2 == '2')
            {
                printf("WIP\n");
            }
            //startConversation();
        }

        // 4 - Criar Grupo
        else if (menu_op1 == '4')
        {
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

                groupname_undefined = 0;
            }

            printf("\n");

            setGroup(groupname, username);
        }

        // 5 - Solicitações
        else if (menu_op1 == '5')
        {
            printf("\n1. Solicitações Recebidas\n"
                   "2. Solicitar Conversa Com Usuário\n"
                   "3. Solicitar Conversa Com Grupo\n"
                   "4. Histórico de solicitações\n\n");
            printf("> ");
            scanf(" %c", &menu_op3);

            // 5.1 - Ver Solicitações
            if (menu_op3 == '1')
            {
                printf("\nBuscando Solicitações Recebidas...\n");

                if(LOG_ENABLED)
                    printf("\n");

                getRequests(username, &control_list, 1);

                if (control_list.head == NULL)
                {
                    printf("\nNenhuma Solicitação Recebida!\n");
                    continue;
                }

                char response;
                printf("\nDeseja Responder Alguma Solicitação? (S/N)\n\n");
                printf("> ");
                scanf(" %c", &response);
                
                if (response != 'S' && response != 's')
                    continue;

                printf("\nSelecione o tipo de solicitação:\n"
                    "1. Usuário\n"
                    "2. Grupo\n\n");
                printf("> ");

                char type;
                scanf(" %c", &type);

                if (type == '1') // Usuário
                {
                    char target_user[64];
                    int target_user_undefined = 1;

                    while (target_user_undefined)
                    {
                        printf("\nDigite o Nome do Usuário: ");
                        scanf("%63s", target_user);

                        if (strlen(target_user) == 0 || strspn(target_user, " \t\n\r") == strlen(target_user))
                        {
                            printf("O Nome do Usuário Não Pode Estar Vazio!\n");
                            continue;
                        }

                        //verifica se o usuario realmente mandou solicitação:
                        int found = 0;
                        Node* node = control_list.head;

                        while (node) {
                            char* msg = node->message;

                            // se começar com "USER_REQUEST:", pula os 13 primeiros caracteres pra pegar só o nome
                            if (strncmp(msg, "USER_REQUEST:", 13) == 0) {
                                msg += 13;
                            }
                            if (strcmp(msg, target_user) == 0) {
                                found = 1;
                                break;
                            }

                            node = node->next;
                        }
                        if (!found) {
                            printf("A Solicitação Não Existe!\n");
                            continue;
                        }

                        target_user_undefined = 0;
                    }   

                    char accept;
                    printf("\nDeseja Aceitar a Solicitação de %s? (S/N)\n\n", target_user);
                    printf("> ");
                    scanf(" %c", &accept);

                    char topic[256];
                    char payload[256];

                    snprintf(topic, sizeof(topic), "%s_Control", target_user); 

                    if (accept == 'S' || accept == 's')
                    {
                        snprintf(payload, sizeof(payload), "REQUEST_ACCEPTED:%s", username);
                        printf("\nSolicitação de %s aceita.\n", target_user);
                    }
                    else
                    {
                        snprintf(payload, sizeof(payload), "REQUEST_REJECTED:%s", username);
                        printf("\nSolicitação de %s rejeitada.\n", target_user);
                    }

                    publisher(username, topic, payload, 0);  //envia resposta da solicitação 

                    //add no histórico:
                    char hist_msg[256];
                    snprintf(hist_msg, sizeof(hist_msg), "USER:%s;%s;%s", 
                            target_user, 
                            (accept=='S' || accept=='s') ? "ACCEPTED" : "REJECTED", 
                            username);
                    listInsert(&received_history, hist_msg);

                    //apagar solicitação
                    snprintf(topic, sizeof(topic), "%s_Control/REQUESTS/USER_REQUEST:%s", username, target_user);
                    publisher(username, topic, "", 1);
                }
                else if (type == '2') // Grupo
                {
                    char target_group[128];
                    char target_user[128];
                    int target_group_undefined = 1;

                    while (target_group_undefined)
                    {
                        printf("\nDigite o Nome do Grupo Solicitado: ");
                        scanf("%63s", target_group);
                        
                        printf("\nDigite o Nome do Usuário Solicitante: ");
                        scanf("%63s", target_user);

                        if (strlen(target_group) == 0 || strspn(target_group, " \t\n\r") == strlen(target_group))
                        {
                            printf("O Nome do Grupo Não Pode Estar Vazio!\n");
                            continue;
                        }

                        //verifica se solicitação existe:
                        int found = 0;
                        Node* node = control_list.head;

                        // monta a string completa pra comparar
                        char request_pattern[512];
                        //padrão: "GROUP_REQUEST:groupName;solicitanteName"
                        snprintf(request_pattern, sizeof(request_pattern), "GROUP_REQUEST:%s;%s", target_group, target_user);

                        while (node) {
                            if (strcmp(node->message, request_pattern) == 0) {
                                found = 1;
                                break;
                            }
                            node = node->next;
                        }

                        if (!found) {
                            printf("Solicitação de grupo inválida!\n");
                            continue;
                        }


                        target_group_undefined = 0;
                    }


                    char accept;
                    printf("\nDeseja Aceitar a Entrada de %s No Grupo %s? (S/N)\n\n", target_user, target_group);
                    printf("> ");
                    scanf(" %c", &accept);

                    char topic[512];
                    char payload[256];

                    snprintf(topic, sizeof(topic), "%s_Control", target_user);
            
                    if (accept == 'S' || accept == 's')
                    {
                        snprintf(payload, sizeof(payload), "GROUP_ACCEPTED:%s;%s", target_group, username);
                        updateGroupMembers(target_group, username, target_user);
                        printf("\nSolicitação de %s para entrar no grupo %s aceita.\n", target_user, target_group);
                    }
                    else
                    {
                        snprintf(payload, sizeof(payload), "GROUP_REJECTED:%s;%s", target_group, username);
                        printf("\nSolicitação de %s para entrar no grupo %s rejeitada.\n", target_user, target_group);
                    }

                    publisher(username, topic, payload, 0); //envia resposta da solicitação 

                    //add solicitação no histórico:
                    char hist_msg[256];
                    snprintf(hist_msg, sizeof(hist_msg), "GROUP:%s;%s;%s", 
                            target_group, 
                            (accept=='S' || accept=='s') ? "ACCEPTED" : "REJECTED", 
                            target_user);
                    listInsert(&received_history, hist_msg);

                    // apagar solicitação de grupo
                    snprintf(topic, sizeof(topic), "%s_Control/REQUESTS/GROUP_REQUEST:%s;%s", username, target_group, target_user);
                    publisher(username, topic, "", 1);

                }
                else {
                    printf("\nOpção Inválida!\n");
                }
            }
        
            // 5.2 - Solicitar (Usuário)
            else if (menu_op3 == '2')
            {
                printf("\nBuscando Usuários Online...\n");

                if(LOG_ENABLED)
                    printf("\n");

                getUsers(username, &status_list, 0);

                listGetOnlineUsers(&status_list, &online_users_list, username);

                if (online_users_list.head == NULL) // No Online Users
                {
                    printf("Nenhum Usuário Online Encontrado!\n");
                    continue;
                }

                listPrint(&online_users_list);

                char user_request = 'N';
                printf("\nDeseja Solicitar Uma Conversa? (S/N)\n\n");
                printf("> ");
                scanf(" %1s", &user_request);

                if (user_request == 'S' || user_request == 's')
                {
                    char target_user[128];
                    int target_user_undefined = 1;

                    while (target_user_undefined) // Validity Checker
                    {
                        printf("\nDigite O Nome Do Usuário Escolhido: ");
                        scanf("%63s", target_user);

                        if (strlen(target_user) == 0 || strspn(target_user, " \t\n\r") == strlen(target_user)) { // No Target User / Only Whitespaces
                            printf("O Nome Do Usuário Não Pode Estar Vazio!\n");
                            continue;
                        }

                        if (listSearch(&online_users_list, target_user) == 0) { // Target User Not Found In Online List
                            printf("O Nome Do Usuário É Inválido!\n");
                            continue;

                        }

                        target_user_undefined = 0;
                    }
                    

                    printf("\n");

                    char temp[256]; // Topic = [TARGET_USER]_Control
                    char request[128]; // USER_REQUEST:[USERNAME]
                    snprintf(temp, sizeof(temp), "%s_Control", target_user);
                    snprintf(request, sizeof(request), "USER_REQUEST:%s", username);

                    publisher(username, temp, request, 0);

                    //add no hist:
                    char hist_msg[256];
                    snprintf(hist_msg, sizeof(hist_msg), "USER:%s;SENT;%s", target_user, username);
                    listInsert(&sent_history, hist_msg);
                }
                else
                {
                    continue; 
                }
            }

            // 5.3 - Solicitar (Grupo)
            else if (menu_op3 == '3')
            {
                printf("\nBuscando Grupos Com Líderes Online...\n");

                getUsers(username, &status_list, 0);
                getGroups(username, &groups_list, 0);

                if(LOG_ENABLED)
                    printf("\n");

                listGetOnlineUsers(&status_list, &online_users_list, username); // First Update Online Users
                listGetOnlineGroups(&online_users_list, &groups_list, &online_groups_list, username);

                if (online_groups_list.head == NULL) // No Online Users
                {
                    printf("Nenhum Líder Online Encontrado!\n");
                    continue;
                }

                listPrintGroups(&online_groups_list);

                char group_request = 'N';
                printf("\nDeseja Solicitar A Entrada Em Um Grupo? (S/N)\n\n");
                printf("> ");
                scanf(" %1s", &group_request);

                if (group_request == 'S' || group_request == 's')
                {
                    char target_group[128];
                    char* target_leader;
                    int target_group_undefined = 1;

                    while (target_group_undefined) // Validity Checker
                    {
                        printf("\nDigite O Nome Do Grupo Escolhido: ");
                        scanf("%63s", target_group);

                        if (strlen(target_group) == 0 || strspn(target_group, " \t\n\r") == strlen(target_group)) { // No Target User / Only Whitespaces
                            printf("O Nome Do Grupo Não Pode Estar Vazio!\n");
                            continue;
                        }

                        if (listSearchFirstParameter(&online_groups_list, target_group) == 0) { // Target Group Not Found In Online List
                            printf("O Nome Do Grupo É Inválido!\n");
                            continue;

                        }

                        target_group_undefined = 0;
                    }
                    

                    printf("\n");

                    printf("Grupo Escolhido: %s", target_group);

                    target_leader = listGetGroupLeader(&groups_list, target_group);
                    printf("Líder: %s", target_leader);


                    char temp[128]; // Topic = [TARGET_LEADER]_Control
                    char request[256]; // GROUP_REQUEST:[GROUPNAME];[USERNAME]
                    snprintf(temp, sizeof(temp), "%s_Control", target_leader);
                    snprintf(request, sizeof(request), "GROUP_REQUEST:%s;%s", target_group, username);

                    publisher(username, temp, request, 0);

                    //add no hist:
                    char hist_msg[256];
                    snprintf(hist_msg, sizeof(hist_msg), "GROUP:%s;SENT;%s", target_group, username);
                    listInsert(&sent_history, hist_msg);
                }
                else
                {
                    continue; 
                }
            }
            // 5.2 - Solicitar (Usuário)
            else if (menu_op3 == '4')
            {
                char hist_choice;
                printf("\nHistórico de Solicitações:\n"
                    "1. Recebidas\n"
                    "2. Enviadas\n\n");
                printf("> ");
                scanf(" %c", &hist_choice);

                if (hist_choice == '1') {
                    printf("\n--- Solicitações Recebidas ---\n");
                    listPrint(&received_history);
                } else if (hist_choice == '2') {
                    printf("\n--- Solicitações Enviadas ---\n");
                    listPrint(&sent_history);
                } else {
                    printf("Opção inválida!\n");
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