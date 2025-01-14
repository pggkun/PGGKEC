#include <malloc.h>

#include "agent.h"

#include <3ds.h>
#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000
static u32 *SOC_buffer = NULL;
s32 sock = -1, csock = -1;

int main()
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    socInit(SOC_buffer, SOC_BUFFERSIZE);

    char *ip = "192.168.0.9";
    //"172.22.128.157"
    printf("ip: '%s'", ip);
    client_agent my_agent(ip);

    //=====================================================
    // MAIN LOOP
    //=====================================================
    while (true)
    {
        gspWaitForVBlank();
		hidScanInput();
        u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break;

        if (kDown & KEY_A)
        {
            message m = {my_agent.uid, 0, 0, "salve do 3ds\n"};
            my_agent.send_message(m);
            // printf("MANDOU\n");
        }
        my_agent.update();
        // printf("ATUALIZOU\n");        
    }
    return 0;
}