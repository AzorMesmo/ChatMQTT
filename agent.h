#ifndef AGENT_H
#define AGENT_H

#include "messages.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Function Declarations */
int agentControl(const char* username_a, LinkedList* control_list, volatile int* online);
void* monitorControlThread(void* arg);

#ifdef __cplusplus
}
#endif

#endif // AGENT_H