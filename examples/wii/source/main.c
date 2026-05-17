#include <gccore.h>
#include <wiiuse/wpad.h>
#include <network.h>
#include <malloc.h>
#include <unistd.h>

#include "pggkec.h"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

s32 sock = -1, csock = -1;

static uint8_t g_ip_octets[4] = {192,168,0,18};
static int g_ip_cursor = 0;

static inline int clamp255(int v)
{
    return v < 0 ? 0 : (v > 255 ? 255 : v);
}

void parse_ip(const char* s, uint8_t out[4])
{
    int a=0,b=0,c=0,d=0;

    if(sscanf(s,"%d.%d.%d.%d",&a,&b,&c,&d)==4)
    {
        out[0]=clamp255(a);
        out[1]=clamp255(b);
        out[2]=clamp255(c);
        out[3]=clamp255(d);
    }
}

void build_ip(char* out, size_t sz, const uint8_t in[4])
{
    snprintf(out, sz, "%u.%u.%u.%u",
             in[0], in[1], in[2], in[3]);
}

void draw_ip_editor()
{
    consoleClear();

    printf("Use D-Pad to edit IP, A to send\n\n");

    for(int i=0;i<4;i++)
    {
        if(i==g_ip_cursor) printf("[");

        printf("%u", g_ip_octets[i]);

        if(i==g_ip_cursor) printf("]");

        if(i!=3) printf(".");
    }

    printf("\n(LEFT/RIGHT select, UP/DOWN change)\n");
}

const char* get_ip(void)
{
    static char ip[16];

    ip[0] = '\0';

    int s = net_socket(AF_INET, SOCK_DGRAM, 0);

    if (s < 0)
        return NULL;

    struct sockaddr_in dst;

    memset(&dst, 0, sizeof(dst));

    dst.sin_family = AF_INET;
    dst.sin_port   = htons(80);

    inet_aton("8.8.8.8", &dst.sin_addr);

    net_connect(s, (struct sockaddr*)&dst, sizeof(dst));

    struct sockaddr_in local;
    socklen_t len = sizeof(local);

    if(net_getsockname(s, (struct sockaddr*)&local, &len) == 0)
    {
        const char* tmp = inet_ntoa(local.sin_addr);

        if(tmp)
        {
            strncpy(ip, tmp, sizeof(ip));
            ip[sizeof(ip)-1] = '\0';
        }
    }

    net_close(s);

    return ip[0] ? ip : NULL;
}

void client_behaviour()
{
    char ip[16];

    build_ip(ip,sizeof(ip),g_ip_octets);

    while(true)
    {
        WPAD_ScanPads();

        u32 down = WPAD_ButtonsDown(0);
        u32 held = WPAD_ButtonsHeld(0);

        if(down & WPAD_BUTTON_LEFT)
            g_ip_cursor = (g_ip_cursor+3)&3;

        if(down & WPAD_BUTTON_RIGHT)
            g_ip_cursor = (g_ip_cursor+1)&3;

        if(down & WPAD_BUTTON_UP)
            g_ip_octets[g_ip_cursor] =
                clamp255(g_ip_octets[g_ip_cursor]+1);

        if(down & WPAD_BUTTON_DOWN)
            g_ip_octets[g_ip_cursor] =
                clamp255(g_ip_octets[g_ip_cursor]-1);

        if(held & WPAD_BUTTON_PLUS)
            g_ip_octets[g_ip_cursor] =
                clamp255(g_ip_octets[g_ip_cursor]+1);

        if(held & WPAD_BUTTON_MINUS)
            g_ip_octets[g_ip_cursor] =
                clamp255(g_ip_octets[g_ip_cursor]-1);

        draw_ip_editor();

        if(down & WPAD_BUTTON_A)
        {
            build_ip(ip,sizeof(ip),g_ip_octets);
            break;
        }

        VIDEO_WaitVSync();
    }

    client_agent *my_agent = create_client_agent(ip);

    while(true)
    {
        WPAD_ScanPads();

        u32 down = WPAD_ButtonsDown(0);

        if(down & WPAD_BUTTON_A)
        {
            message *m = malloc(sizeof(message));

            *m = (message)
            {
                my_agent->uid,
                0,
                0,
                (unsigned char)"Hello from Wii"
            };

            queue_enqueue(my_agent->to_send_non_reliable, m);
        }

        update_client_agent(my_agent);

        if(down & WPAD_BUTTON_HOME)
        {
            break;
        }

        VIDEO_WaitVSync();
    }

    printf("Closing client...\n");

    destroy_client_agent(my_agent);

    consoleClear();
}

void server_behaviour()
{
    server_agent *my_agent = create_server_agent();

    while(true)
    {
        WPAD_ScanPads();

        u32 down = WPAD_ButtonsDown(0);

        if(down & WPAD_BUTTON_A)
        {
            message *m = malloc(sizeof(message));

            *m = (message)
            {
                my_agent->uid,
                0,
                0,
                (unsigned char)"Hello from Wii Server"
            };

            server_send_message(my_agent, m);
        }

        server_update(my_agent);

        if(down & WPAD_BUTTON_HOME)
        {
            break;
        }

        VIDEO_WaitVSync();
    }

    printf("Closing server...\n");

    destroy_server_agent(my_agent);

    consoleClear();
}

int main(int argc, char **argv)
{
    VIDEO_Init();
    WPAD_Init();

    rmode = VIDEO_GetPreferredMode(NULL);

    xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));

    console_init(
        xfb,
        20,
        20,
        rmode->fbWidth,
        rmode->xfbHeight,
        rmode->fbWidth * VI_DISPLAY_PIX_SZ
    );

    VIDEO_Configure(rmode);

    VIDEO_SetNextFramebuffer(xfb);

    VIDEO_SetBlack(FALSE);

    VIDEO_Flush();

    VIDEO_WaitVSync();

    if(rmode->viTVMode & VI_NON_INTERLACE)
        VIDEO_WaitVSync();

    char localip[16] = {0};
	char gateway[16] = {0};
	char netmask[16] = {0};
    int ret = if_config ( localip, netmask, gateway, TRUE, 20);
	if (ret>=0)
		printf ("network configured, ip: %s, gw: %s, mask %s\n", localip, gateway, netmask);

    const char *ip_str = get_ip();

    while(true)
    {
        bool is_server = false;

        printf(
            "IP: %s\n"
            "Press (B) - Server\n"
            "Press (A) - Client\n",
            ip_str ? ip_str : "Unavailable"
        );

        while(true)
        {
            WPAD_ScanPads();

            u32 down = WPAD_ButtonsDown(0);

            if(down & WPAD_BUTTON_B)
            {
                is_server = true;
                break;
            }
            else if(down & WPAD_BUTTON_A)
            {
                break;
            }
            else if(down & WPAD_BUTTON_HOME)
            {
                return 0;
            }

            VIDEO_WaitVSync();
        }

        consoleClear();

        if(is_server)
            server_behaviour();
        else
            client_behaviour();
    }

    return 0;
}