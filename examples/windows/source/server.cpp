#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include "pggkec.h"

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) 
    {
        std::cerr << "Falha ao inicializar o WinSock. Erro: " << WSAGetLastError() << std::endl;
        return 1;
    }

    get_device_ip();
    server_agent my_agent;

    //======================================================
    // START THREAD DE INPUT
    //======================================================
    thread_t thread_id;
    void * thread_res;
    int rstatus;
    
    THREAD_CREATE(&thread_id, update_messages, &my_agent);
 
    printf ("Thread criado com sucesso!\n");
    //=====================================================
    // MAIN LOOP
    //=====================================================
    while (true)
    {
        my_agent.update();
    }

    //=====================================================
    // FINALIZANDO THREAD DE INPUT
    //=====================================================
    THREAD_JOIN(&thread_id);
    //=====================================================

    WSACleanup();
    return 0;
}