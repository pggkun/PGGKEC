#include <malloc.h>
#include <stdio.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/ctrl.h>
#include <psp2/net/net.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
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

static uint8_t g_ip_octets[4] = {192,168,0,18};
static int g_ip_cursor = 0;
static uint32_t g_prev_buttons = 0;

static inline int clamp255(int v){ return v<0?0:(v>255?255:v); }

void parse_ip(const char* s, uint8_t out[4])
{
    int a=0,b=0,c=0,d=0;
    if(sscanf(s,"%d.%d.%d.%d",&a,&b,&c,&d)==4){ out[0]=clamp255(a); out[1]=clamp255(b); out[2]=clamp255(c); out[3]=clamp255(d); }
}

void build_ip(char* out, size_t sz, const uint8_t in[4])
{
    snprintf(out, sz, "%u.%u.%u.%u", in[0], in[1], in[2], in[3]);
}

void draw_ip_editor(void)
{
    printf("\x1b[H\x1b[2J"); // clear
    printf("Use D-Pad to edit IP, CROSS to send\n\n");
    for(int i=0;i<4;i++){
        if(i==g_ip_cursor) printf("[");
        printf("%u", g_ip_octets[i]);
        if(i==g_ip_cursor) printf("]");
        if(i!=3) printf(".");
    }
    printf("\n(LEFT/RIGHT select, UP/DOWN change)\n");
}

void update_ip_editor(const SceCtrlData* ctrl)
{
    uint32_t pressed = ctrl->buttons & ~g_prev_buttons;
    g_prev_buttons = ctrl->buttons;

    if(pressed & SCE_CTRL_LEFT)  g_ip_cursor = (g_ip_cursor+3)&3;
    if(pressed & SCE_CTRL_RIGHT) g_ip_cursor = (g_ip_cursor+1)&3;
    if(pressed & SCE_CTRL_UP)    g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(g_ip_octets[g_ip_cursor]+1);
    if(pressed & SCE_CTRL_DOWN)  g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(g_ip_octets[g_ip_cursor]-1);
}

int main(int argc, char *argv[])
{
	psvDebugScreenInit();

	static char net_mem[1*1024*1024];
	sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    SceNetInitParam netInitParam = { net_mem, 1 * 1024 * 1024 };
    sceNetInit(&netInitParam);

    SceCtrlData ctrl={};

    char ip[16]; build_ip(ip,sizeof(ip),g_ip_octets);

    while(1)
    {
        sceCtrlPeekBufferPositive(0, &ctrl, 1);
        update_ip_editor(&ctrl);
        draw_ip_editor();

        if(ctrl.buttons == SCE_CTRL_CROSS)
        {
            build_ip(ip,sizeof(ip),g_ip_octets);
            break;
        }
        
    }

    client_agent *my_agent = create_client_agent(ip);

    while (true)
    {      
        sceCtrlReadBufferPositive(0,&ctrl,1);
        if(ctrl.buttons == SCE_CTRL_CROSS)
        {
            message *m = malloc(sizeof(message));
            *m = (message){my_agent->uid, 0, 0, "Hello from Vita\n"};
            queue_enqueue(my_agent->to_send_non_reliable, m);
        }
        
        update_client_agent(my_agent);
    }

	sceNetTerm();
	sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);
	sceKernelDelayThread(~0);
    destroy_client_agent(my_agent);
    return 0;
}
