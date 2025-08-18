#include <sys/types.h>
#include "pggkec.h"

int main()
{
    server_agent *s_agent = create_server_agent();
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
    return 0;
}