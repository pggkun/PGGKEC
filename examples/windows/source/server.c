#include <sys/types.h>
#include "pggkec.h"

void process_message(void *agent, void *m)
{
    server_agent *m_agent = (server_agent *) agent;
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

    server_agent *s_agent = create_server_agent();
    set_server_callback(s_agent, process_message);

    thread_t thread_id;
    void * thread_res;
    int rstatus;
    
    THREAD_CREATE(&thread_id, update_messages_as_server, s_agent);

    while(1)
    {
        server_update(s_agent);
    }
    
    THREAD_JOIN(&thread_id);

    destroy_server_agent(s_agent);
    WSACleanup();
    return 0;
}
