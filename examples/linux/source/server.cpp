#include "pggkec.h"

int main()
{
    // socket_t server_sockfd = create_server_socket();
    server_agent my_agent;
    // my_agent.m_sockfd = &server_sockfd;

    //======================================================
    // START THREAD DE INPUT
    //======================================================
    pthread_t thread_id;
    void * thread_res;
    int rstatus;
    
    rstatus = pthread_create(&thread_id, NULL, update_messages, &my_agent);
 
    if(rstatus != 0)
    {
        printf ("Erro ao criar o thread.\n");
        exit(EXIT_FAILURE);
    }
 
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
    rstatus = pthread_join (thread_id, &thread_res);

    if(rstatus != 0)
    {
        printf ("Erro ao aguardar finalização do thread.\n");
        exit(EXIT_FAILURE);
    }
 
    printf ("Thread finalizado! Retorno = %s\n", (char *)thread_res);
    //=====================================================

    return 0;
}