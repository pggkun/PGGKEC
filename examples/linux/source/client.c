#include <sys/types.h>
#include "pggkec.h"

int main()
{
    printf("Insert server ip: ");

    char ip_str[16];
    if (!fgets(ip_str, 16, stdin)) return 0;
    size_t len = strcspn(ip_str, "\r\n");
    ip_str[len] = '\0';

    client_agent *c_agent = create_client_agent(ip_str);

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
    return 0;
}
