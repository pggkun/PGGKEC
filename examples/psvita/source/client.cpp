#include <malloc.h>
#include <stdio.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <psp2/net/net.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


#include "debugScreen.h"
#define printf psvDebugScreenPrintf
#include "pggkec.h"


int main(int argc, char *argv[])
{
	psvDebugScreenInit();

	static char net_mem[1*1024*1024];
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    SceNetInitParam netInitParam = { net_mem, 1 * 1024 * 1024 };
    sceNetInit(&netInitParam);

    char *ip = "192.168.0.12";
    printf("ip: '%s'\n", ip);
    client_agent my_agent(ip);

    SceCtrlData ctrl={};
    while (true)
    {      
        sceCtrlReadBufferPositive(0,&ctrl,1);
        if(ctrl.buttons == SCE_CTRL_LTRIGGER)
        {
            message m = {my_agent.uid, 0, 0, "Hello from Vita\n"};
            my_agent.send_message(m);
        }
        
        my_agent.update();
    }

	sceNetTerm();
	sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
	sceKernelDelayThread(~0);
    return 0;
}
