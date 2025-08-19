#include <malloc.h>
#include "pggkec.h"
#include <3ds.h>

#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x100000
static u32 *SOC_buffer = NULL;
s32 sock = -1, csock = -1;

static uint8_t g_ip_octets[4]={192,168,0,18};
static int g_ip_cursor=0;

static inline int clamp255(int v){ return v<0?0:(v>255?255:v); }

void parse_ip(const char* s, uint8_t out[4])
{
    int a=0,b=0,c=0,d=0; if(sscanf(s,"%d.%d.%d.%d",&a,&b,&c,&d)==4){ out[0]=clamp255(a); out[1]=clamp255(b); out[2]=clamp255(c); out[3]=clamp255(d); }
}
void build_ip(char* out, size_t sz, const uint8_t in[4])
{
    snprintf(out, sz, "%u.%u.%u.%u", in[0], in[1], in[2], in[3]);
}
void draw_ip_editor()
{
    consoleClear();
    printf("Use D-Pad to edit IP, A to send\n\n");
    for(int i=0;i<4;i++){
        if(i==g_ip_cursor) printf("[");
        printf("%u", g_ip_octets[i]);
        if(i==g_ip_cursor) printf("]");
        if(i!=3) printf(".");
    }
    printf("\n(LEFT/RIGHT select, UP/DOWN change)\n");
}

int main()
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    socInit(SOC_buffer, SOC_BUFFERSIZE);

    char ip[16]; build_ip(ip,sizeof(ip),g_ip_octets);

    while(true)
    {
        hidScanInput();
        u32 kDown = hidKeysDown();

        if(kDown & KEY_DLEFT)  g_ip_cursor = (g_ip_cursor+3)&3;
        if(kDown & KEY_DRIGHT) g_ip_cursor = (g_ip_cursor+1)&3;
        if(kDown & KEY_DUP)    g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(g_ip_octets[g_ip_cursor]+1);
        if(kDown & KEY_DDOWN)  g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(g_ip_octets[g_ip_cursor]-1);

        draw_ip_editor();
        

        if(kDown & KEY_A)
        {
            build_ip(ip,sizeof(ip),g_ip_octets);
            break;
        }

        gspWaitForVBlank();
        gfxFlushBuffers();
        gfxSwapBuffers();
    }
    client_agent *my_agent = create_client_agent(ip);
    printf("Pointing to %s\n", ip);

    while (true)
    {
        gspWaitForVBlank();
		hidScanInput();
        u32 kDown = hidKeysDown();
		if (kDown & KEY_START) break;

        if (kDown & KEY_A)
        {
            message *m = malloc(sizeof(message));
            *m = (message){my_agent->uid, 0, 0, "Hello from 3DS\n"};
            queue_enqueue(my_agent->to_send_non_reliable, m);
        }
        update_client_agent(my_agent); 
    }

    destroy_client_agent(my_agent);
    return 0;
}