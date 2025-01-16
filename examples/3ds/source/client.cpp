#include <malloc.h>

#include "pggkec.h"

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

    std::string server_found = server_discovery();
    printf("server found at: %s\n", server_found.c_str());

    client_agent my_agent(server_found.c_str());

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
        }
        my_agent.update();   
    }
    return 0;
}