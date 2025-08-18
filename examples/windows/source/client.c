#include <sys/types.h>
#include "pggkec.h"

void process_message(void *agent, void *m)
{
    client_agent *m_agent = (client_agent *) agent;
    message *m_message = (message *) m;
    printf("processing message: %s\n", m_message->data);
    //TODO: process message here
}

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        printf("Failed initializing WinSock. Erro: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Insert server ip: ");

    char ip_str[16];
    if (!fgets(ip_str, 16, stdin)) return 0;
    size_t len = strcspn(ip_str, "\r\n");
    ip_str[len] = '\0';

    client_agent *c_agent = create_client_agent(ip_str);
    set_client_callback(c_agent, process_message);

    thread_t thread_id;
    void * thread_res;
    int rstatus;
    
    THREAD_CREATE(&thread_id, update_messages_as_client, c_agent);

    while(1)
    {
        update_client_agent(c_agent);
    }
    
    THREAD_JOIN(&thread_id);

    destroy_client_agent(c_agent);
    WSACleanup();
    return 0;
}
