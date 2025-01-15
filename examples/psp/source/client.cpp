#include <arpa/inet.h>
#include <errno.h>
#include <pspctrl.h>
#include <pspkernel.h>
#include <pspnet_apctl.h>
#include <pspsdk.h>
#include <psputility.h>
#include <string.h>
#include <unistd.h>
#include "pggkec.h"

#define printf pspDebugScreenPrintf

#define MODULE_NAME "PGGKEC PSP Example"

PSP_MODULE_INFO(MODULE_NAME, 0, 1, 1);
PSP_HEAP_THRESHOLD_SIZE_KB(1024);
PSP_HEAP_SIZE_KB(-2048);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);
PSP_MAIN_THREAD_STACK_SIZE_KB(1024);

/* Exit callback */
int exit_callback(int arg1, int arg2, void *common)
{
	sceKernelExitGame();
	return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp)
{
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();

	return 0;
}

/* Sets up the callback thread and returns its thread id */
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

/* Connect to an access point */
int connect_to_apctl(int config)
{
	int err;
	int stateLast = -1;

	/* Connect using the first profile */
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
			break; // connected with static IP

		// wait a little before polling again
		sceKernelDelayThread(50 * 1000); // 50ms
	}
	printf(MODULE_NAME ": Connected!\n");

	if (err != 0)
	{
		return 0;
	}

	return 1;
}

int main(int argc, char **argv)
{
	SceUID thid;

	SetupCallbacks();

	sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
	sceUtilityLoadNetModule(PSP_NET_MODULE_INET);

	pspDebugScreenInit();

	int err;
	if ((err = pspSdkInetInit()))
	{
		printf(MODULE_NAME ": Error, could not initialise the network %08X\n", err);
		return 0;
	}

	if (connect_to_apctl(1))
	{
		// connected, get my IPADDR and run test
		union SceNetApctlInfo info;

		if (sceNetApctlGetInfo(8, &info) != 0)
			strcpy(info.ip, "unknown IP");

		get_device_ip();

		char *ip = "192.168.0.9";
		//"172.22.128.157"
		printf("ip: '%s'\n", ip);
		client_agent my_agent(ip);
		printf("client created!\n");

		SceCtrlData ctrl;
		while (true)
		{   
			sceCtrlReadBufferPositive(&ctrl,1);
			if(ctrl.Buttons & PSP_CTRL_CROSS)
			{
				message m;
				m.source = my_agent.uid;
				m.destiny = 0;
				m.index = 0;
				strcpy(m.data, "salve do PSP\n");

				my_agent.send_message(m);
			}
			
			my_agent.update();
		}

	}

	return 0;
}
