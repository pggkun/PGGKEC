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

const char* get_ip(void) 
{
    static char ip[16]; // "255.255.255.255\0"
    ip[0] = '\0';

    int s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) return NULL;

    struct sockaddr_in dst;
    memset(&dst, 0, sizeof(dst));
    dst.sin_family = AF_INET;
    dst.sin_port   = htons(80);
    inet_aton("8.8.8.8", &dst.sin_addr);

    (void)connect(s, (struct sockaddr*)&dst, sizeof(dst));

    struct sockaddr_in local;
    socklen_t len = sizeof(local);
    if (getsockname(s, (struct sockaddr*)&local, &len) == 0)
    {
        const char* tmp = inet_ntoa(local.sin_addr);
        if (tmp) {
            strncpy(ip, tmp, sizeof(ip));
            ip[sizeof(ip)-1] = '\0';
        }
    }

    close(s);
    return ip[0] ? ip : NULL;
}

void client_behaviour()
{
    char ip[16]; build_ip(ip,sizeof(ip),g_ip_octets);

    while(true)
    {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();

        if(kDown & KEY_DLEFT)  g_ip_cursor = (g_ip_cursor+3)&3;
        if(kDown & KEY_DRIGHT) g_ip_cursor = (g_ip_cursor+1)&3;
        if(kDown & KEY_DUP)    g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(g_ip_octets[g_ip_cursor]+1);
        if(kDown & KEY_DDOWN)  g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(g_ip_octets[g_ip_cursor]-1);
        if(kHeld & KEY_R)    g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(g_ip_octets[g_ip_cursor]+1);
        if(kHeld & KEY_L)  g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(g_ip_octets[g_ip_cursor]-1);

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

    while (true)
    {
        gspWaitForVBlank();
		hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_A)
        {
            message *m = malloc(sizeof(message));
            *m = (message){my_agent->uid, 0, 0, "Hello from 3DS"};
            queue_enqueue(my_agent->to_send_non_reliable, m);
        }

        update_client_agent(my_agent);

        if (kDown & KEY_START) 
        {
            break;
        }
    }

    printf("Closing client...\n");
	destroy_client_agent(my_agent);
    consoleClear();
}

void server_behaviour()
{
    server_agent *my_agent = create_server_agent();

    while (true)
    {
        gspWaitForVBlank();
		hidScanInput();
        u32 kDown = hidKeysDown();

        if (kDown & KEY_A)
        {
            message *m = malloc(sizeof(message));
            *m = (message){my_agent->uid, 0, 0, "Hello from 3DS Server"};
            server_send_message(my_agent, m);
        }
        server_update(my_agent);

        if (kDown & KEY_START) 
        {
            break;
        }
    }

    printf("Closing server...\n");
	destroy_server_agent(my_agent);
    consoleClear();
}

int main()
{
    gfxInitDefault();
    consoleInit(GFX_TOP, NULL);
    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    socInit(SOC_buffer, SOC_BUFFERSIZE);

    const char *ip_str = get_ip();

    while(true)
    {
        bool is_server = false;
        printf("IP: %s\nPress (B) - Server\nPress (A) - Client\n", ip_str);
        while(true)
        {
            gspWaitForVBlank();
            hidScanInput();
            u32 kDown = hidKeysDown();

            if(kDown & KEY_B)
            {
                is_server = true;
                break;
            }
            else if(kDown & KEY_A)
            {
                break;
            }
            else if(kDown & KEY_START)
            {
                return 0;
            }
        }
        consoleClear();

        if(is_server)
            server_behaviour();
        else
            client_behaviour();
    }

    return 0;
}