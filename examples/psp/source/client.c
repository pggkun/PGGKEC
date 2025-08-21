#include <arpa/inet.h>
#include <errno.h>
#include <pspctrl.h>
#include <pspkernel.h>
#include <pspnet_apctl.h>
#include <pspsdk.h>
#include <psputility.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "pggkec.h"
#include <psputility_netconf.h>
#include <pspdisplay.h>
#include <psppower.h>

#include <pspnet.h>
#include <pspnet_inet.h>

static uint8_t g_ip_octets[4] = {192, 168, 0, 29};
static int g_ip_cursor = 0;
static uint32_t g_prev_buttons = 0;

static inline int clamp255(int v) { return v < 0 ? 0 : (v > 255 ? 255 : v); }

void parse_ip(const char* s, uint8_t out[4]) {
    int a=0,b=0,c=0,d=0;
    if(sscanf(s, "%d.%d.%d.%d", &a,&b,&c,&d) == 4) {
        out[0]=clamp255(a); out[1]=clamp255(b); out[2]=clamp255(c); out[3]=clamp255(d);
    }
}

void build_ip(char* out, size_t sz, const uint8_t in[4])
{
    snprintf(out, sz, "%u.%u.%u.%u", in[0], in[1], in[2], in[3]);
}

void draw_ip_editor(void)
{
    pspDebugScreenSetXY(0, 2);
    for (int i = 0; i < 4; ++i) {
        if (i == g_ip_cursor) printf("[");
        printf("%u", g_ip_octets[i]);
        if (i == g_ip_cursor) printf("]");
        if (i != 3) printf(".");
    }
    printf("   (LEFT/RIGHT select, UP/DOWN change)");
}

void update_ip_editor(const SceCtrlData* ctrl)
{
    uint32_t pressed = ctrl->Buttons & ~g_prev_buttons;
    g_prev_buttons = ctrl->Buttons;

    if (pressed & PSP_CTRL_LEFT)  { g_ip_cursor = (g_ip_cursor + 3) & 3; }
    if (pressed & PSP_CTRL_RIGHT) { g_ip_cursor = (g_ip_cursor + 1) & 3; }

    if (pressed & PSP_CTRL_UP) {
        int v = g_ip_octets[g_ip_cursor];
        g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(v + 1);
    }
    if (pressed & PSP_CTRL_DOWN) {
        int v = g_ip_octets[g_ip_cursor];
        g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(v - 1);
    }
	if(ctrl->Buttons & PSP_CTRL_RTRIGGER) {
        int v = g_ip_octets[g_ip_cursor];
        g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(v + 1);
    }
    if(ctrl->Buttons & PSP_CTRL_LTRIGGER) {
        int v = g_ip_octets[g_ip_cursor];
        g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(v - 1);
    }
}

int netconf_connect_blocking(void) 
{
	sceDisplaySetMode(0, 480, 272);
	sceDisplaySetFrameBuf((void*)0x44000000, 512, PSP_DISPLAY_PIXEL_FORMAT_8888, PSP_DISPLAY_SETBUF_NEXTFRAME);
    pspUtilityNetconfData data;
    memset(&data, 0, sizeof(data));
    data.base.size = sizeof(data);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_LANGUAGE, &data.base.language);
    sceUtilityGetSystemParamInt(PSP_SYSTEMPARAM_ID_INT_UNKNOWN,  &data.base.buttonSwap);
    data.base.graphicsThread = 0x11;
    data.base.accessThread   = 0x13;
    data.base.fontThread     = 0x12;
    data.base.soundThread    = 0x10;
    data.action = PSP_NETCONF_ACTION_CONNECTAP; // diálogo de conexão

    sceUtilityNetconfInitStart(&data);

    int status;
    do {
        status = sceUtilityNetconfGetStatus();
        switch (status) {
            case PSP_UTILITY_DIALOG_VISIBLE: sceUtilityNetconfUpdate(1); break;
            case PSP_UTILITY_DIALOG_QUIT:    sceUtilityNetconfShutdownStart(); break;
        }
		sceDisplayWaitVblankStart();
        sceKernelDelayThread(10 * 1000);
    } while (status != PSP_UTILITY_DIALOG_NONE);

    return 0;
}

#define printf pspDebugScreenPrintf

#define MODULE_NAME "PGGKEC PSP Example"

PSP_MODULE_INFO(MODULE_NAME, 0, 1, 1);
PSP_HEAP_THRESHOLD_SIZE_KB(1024);
PSP_HEAP_SIZE_KB(-2048);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);
PSP_MAIN_THREAD_STACK_SIZE_KB(1024);

int exit_callback(int arg1, int arg2, void *common)
{
	sceKernelExitGame();
	return 0;
}

int CallbackThread(SceSize args, void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();

	return 0;
}

int SetupCallbacks(void)
{
	int thid = 0;

	thid =
			sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, PSP_THREAD_ATTR_USER, 0);
	if (thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}

	return thid;
}

int connect_to_apctl(int config)
{
	int err;
	int stateLast = -1;

	err = sceNetApctlConnect(config);
	if (err != 0)
	{
		printf(MODULE_NAME ": sceNetApctlConnect returns %08X\n", err);
		return 0;
	}

	printf(MODULE_NAME ": Connecting...\n");
	while (1)
	{
		int state;
		err = sceNetApctlGetState(&state);
		if (err != 0)
		{
			printf(MODULE_NAME ": sceNetApctlGetState returns $%x\n", err);
			break;
		}
		if (state > stateLast)
		{
			printf("  connection state %d of 4\n", state);
			stateLast = state;
		}
		if (state == 4)
			break;

		sceKernelDelayThread(500 * 1000);
	}
	printf(MODULE_NAME ": Connected!\n");

	if (err != 0)
	{
		return 0;
	}

	return 1;
}

void client_behaviour()
{
	char ip_str[16];

	SceCtrlData ctrl;
	while(true)
	{
		sceCtrlReadBufferPositive(&ctrl,1);
		update_ip_editor(&ctrl);
		draw_ip_editor();

		if(ctrl.Buttons & PSP_CTRL_CROSS)
		{
			build_ip(ip_str, sizeof(ip_str), g_ip_octets);
			break;
		}
	}
	pspDebugScreenClear();

	printf("server ip:  '%s'\n", ip_str);
	client_agent *my_agent = create_client_agent(ip_str);

	while (true)
	{   
		sceCtrlReadBufferPositive(&ctrl,1);
		uint32_t pressed = ctrl.Buttons & ~g_prev_buttons;
		g_prev_buttons = ctrl.Buttons;

		if(pressed & PSP_CTRL_CROSS)
		{
			message *m = malloc(sizeof(message));
			m->source = my_agent->uid;
			m->destiny = 0;
			m->index = 0;
			strcpy(m->data, "Hello from PSP");

			queue_enqueue(my_agent->to_send_non_reliable, m);
		}

		update_client_agent(my_agent);

		if(pressed & PSP_CTRL_START)
		{
			break;
		}
	}
	printf("Closing client...\n");
	destroy_client_agent(my_agent);
	pspDebugScreenClear();
}

void server_behaviour()
{
	SceCtrlData ctrl;
	server_agent *my_agent = create_server_agent();

	while (true)
	{   
		sceCtrlReadBufferPositive(&ctrl,1);
		uint32_t pressed = ctrl.Buttons & ~g_prev_buttons;
		g_prev_buttons = ctrl.Buttons;

		if(pressed & PSP_CTRL_CROSS)
		{
			message m;
			m.source = my_agent->uid;
			m.destiny = 0;
			m.index = 0;
			strcpy(m.data, "Hello from PSP Server");

			server_send_message(my_agent, &m);
		}
		
		server_update(my_agent);

		if(pressed & PSP_CTRL_START)
		{
			break;
		}
	}
	printf("Closing server...\n");
	destroy_server_agent(my_agent);
	pspDebugScreenClear();
}

int main(int argc, char **argv)
{
	SceUID thid;

	SetupCallbacks();

	pspDebugScreenInit();
	int err;

	err = sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
    if (err < 0) { printf(MODULE_NAME ": Load COMMON failed 0x%08X\n", err); return 0; }

    err = sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
    if (err < 0) { printf(MODULE_NAME ": Load INET failed 0x%08X\n", err); return 0; }

    printf(MODULE_NAME ": net_init OK\n");

	if ((err = pspSdkInetInit()))
	{
		printf(MODULE_NAME ": Error, could not initialise the network %08X\n", err);
		return 0;
	}

	if(!connect_to_apctl(1))
	{
		netconf_connect_blocking();
	}
	pspDebugScreenClear();

	if (true)
	{
		union SceNetApctlInfo info;

		if (sceNetApctlGetInfo(8, &info) != 0)
			strcpy(info.ip, "unknown IP");

		SceCtrlData ctrl;
		
		while(true)
		{
			bool is_server = false;
			printf("IP: %s\nPress (X) - Server\nPress (O) - Client\n", info.ip);
			while(true)
			{
				sceCtrlReadBufferPositive(&ctrl,1);
				uint32_t pressed = ctrl.Buttons & ~g_prev_buttons;
				g_prev_buttons = ctrl.Buttons;

				if(pressed & PSP_CTRL_CROSS)
				{
					is_server = true;
					break;
				}
				else if(pressed & PSP_CTRL_CIRCLE)
				{
					break;
				}
				else if(pressed & PSP_CTRL_START)
				{
					return 0;
				}
			}
			pspDebugScreenClear();

			if(is_server)
				server_behaviour();
			else
				client_behaviour();
		}
	}

	return 0;
}
