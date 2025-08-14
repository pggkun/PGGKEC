#include "pggkec.h"
#include <stdarg.h>
#include <switch.h>

void cprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    consoleUpdate(NULL);
}

int main(int argc, char **argv)
{
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    PadState pad;
    padInitializeDefault(&pad);

    socketInitializeDefault();

    char *ip = "192.168.0.12";
    cprintf("server ip: '%s'\n", ip);

    client_agent my_agent(ip);
    cprintf("client created\n");

    pthread_t thread_id;
    void * thread_res;
    int rstatus;
    
    rstatus = pthread_create(&thread_id, NULL, update_messages, &my_agent);
    if(rstatus != 0)
    {
        cprintf ("Error creating thread.\n");
        exit(EXIT_FAILURE);
    }
    cprintf ("Thread created successfully!\n");

    while(appletMainLoop())
    {
        padUpdate(&pad);
        u32 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break;

        if (kDown & HidNpadButton_A) 
        {
            message m;
            m.source = my_agent.uid;
            m.destiny = 0;
            m.index = 0;
            strcpy(m.data, "Hello from Switch\n");

            my_agent.send_message(m);
        }
        my_agent.update();

        consoleUpdate(NULL);
    }

    rstatus = pthread_join(thread_id, &thread_res);
    if(rstatus != 0)
    {
        cprintf ("Error while waiting for the thread to finish.\n");
        exit(EXIT_FAILURE);
    }
    cprintf ("Thread finished! Return = %s\n", (char *)thread_res);

    socketExit();
    consoleExit(NULL);
    return 0;
}
