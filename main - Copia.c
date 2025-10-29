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


typedef struct { //Groups
    char name[64];
    char leader[64];
    char members[10][64]; // Exemplo: até 10 membros
    int members_count;
} Group;
Group groups[10];
int groups_count = 0;

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
    publishStatus(leaderName, "GROUPS", payload);
    printf("Grupo '%s' criado com líder '%s'\n", groupName, leaderName);
}

void listGroups() {
    printf("Grupos cadastrados:\n");
    if (groups_count == 0) {
        printf("Nenhum grupo cadastrado.\n");
        return;
    }

    for (int i = 0; i < groups_count; i++) {
        printf("Grupo: %s\n", groups[i].name);
        printf("  Líder: %s\n", groups[i].leader);
        printf("  Membros: ");
        for (int j = 0; j < groups[i].members_count; j++) {
            printf("%s", groups[i].members[j]);
            if (j < groups[i].members_count - 1) printf(", ");
        }
        printf("\n");
    }
}

void startConversation() {
    printf("Funcionalidade de conversa ainda não implementada.\n");
}

void showRequestHistory() {
    printf("Histórico.\n");
}


int main()
{
    // Welcome & Username Definition

    char username[64];
    printf("ChatMQTT\n\n\nDigite Seu Nome De Usuário (Máximo 63 Caractéres): ");
    scanf("%63s", username); // Limit Username Input To 63 Characters + '\0'
    char controlTopic[128];
    snprintf(controlTopic, sizeof(controlTopic), "%s_Control", username);// Define Control Topic Based On Username
    printf("Bem Vindo, %s!\n\n\n", username);

    // Threads Parameters

    pthread_t threads[1]; // Threads Handler
    int threads_running = 0; // Threads Counter

    // Send Online Status (USERS Topic)

    PublishArgs pubArgsControl; // Function statusHeartbeat Arguments
    strncpy(pubArgsControl.username, username, 64);
    strncpy(pubArgsControl.topic, controlTopic, 64);
    snprintf(pubArgsControl.payload, sizeof(pubArgsControl.payload), "%s: Ready", username);

    pthread_t subscriber_thread;
    if (pthread_create(&subscriber_thread, NULL, (void*(*)(void*))main_subscribe, NULL) != 0) {
        perror("Failed To Create The Subscriber");
        return 1;
    }


    threads_running += 1;
    if (pthread_create(&threads[0], NULL, statusHeartbeat, &pubArgsControl) != 0) // Status Heartbeat
    {
        perror("Failed To Create The Subscriber");
        threads_running -= 1;
        return 1;
    }

    //MENU
    char op;
    char op2;
    char op3;
    char groupName[64];

    while (1) {
        printf("Menu:\n1. Listar Usuários\n2. Listar Grupos\n3. Conversar\n4. Criar Grupo\n5. Ver histórico de Solicitações\n6. Sair\n\n");
        scanf(" %c", &op); 

        if(op == '1'){
            //online offline
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
            printf("Ver histórico de:\n1. Solicitações de conversas enviadas\n2. Solicitações de conversas recebidas\n3. Solicitações para meus grupos\n");
            scanf(" %c", &op3); 
            showRequestHistory();
        }
        else if(op == '6'){
            printf("Saindo...\n");
            break;
        }
        else{
            printf("Opção Inválida!\n");
        }
    }

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
    char payload[1024];
    snprintf(payload, sizeof(payload), "%s: Offline", username);
    publishStatus(username, "USERS", payload);


    return 0;
}