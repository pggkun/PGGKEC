#include "pggkec.h"
#include <stdarg.h>
#include <stdbool.h>
#include <switch.h>

void cprintf(const char *fmt, ...) 
{
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    consoleUpdate(NULL);
}

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
void draw_ip_editor(void)
{
    consoleClear();
    cprintf("Use D-Pad to edit IP, A to send\n\n");
    for(int i=0;i<4;i++){
        if(i==g_ip_cursor) printf("[");
        printf("%u", g_ip_octets[i]);
        if(i==g_ip_cursor) printf("]");
        if(i!=3) printf(".");
    }
    cprintf("\n(LEFT/RIGHT select, UP/DOWN change)\n");
}

void print_local_ip(void) 
{
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        cprintf("Error creating socket\n");
        return;
    }

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_port = htons(80);

    inet_pton(AF_INET, "8.8.8.8", &serv.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv, sizeof(serv)) < 0) 
    {
        cprintf("Connection failure\n");
        close(sock);
        return;
    }

    struct sockaddr_in local;
    socklen_t len = sizeof(local);
    if (getsockname(sock, (struct sockaddr*)&local, &len) == -1) 
    {
        cprintf("Fail to retrieve current address\n");
        close(sock);
        return;
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &local.sin_addr, ip_str, sizeof(ip_str));
    cprintf("Device IP: %s\n", ip_str);

    close(sock);
}

int main(int argc, char **argv)
{
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);

    PadState pad;
    padInitializeDefault(&pad);

    socketInitializeDefault();

    print_local_ip();

    char ip[16]; build_ip(ip,sizeof(ip),g_ip_octets);

    while(true)
    {
        padUpdate(&pad);

        u64 kDown = padGetButtonsDown(&pad);
        u64 kHeld = padGetButtons(&pad);

        if (kDown & HidNpadButton_Left)  g_ip_cursor = (g_ip_cursor + 3) & 3;
        if (kDown & HidNpadButton_Right) g_ip_cursor = (g_ip_cursor + 1) & 3;

        if (kDown & HidNpadButton_Up)
            g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(g_ip_octets[g_ip_cursor] + 1);
        if (kDown & HidNpadButton_Down)
            g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(g_ip_octets[g_ip_cursor] - 1);
        if (kHeld & HidNpadButton_R)
            g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(g_ip_octets[g_ip_cursor] + 1);
        if (kHeld & HidNpadButton_L)
            g_ip_octets[g_ip_cursor] = (uint8_t)clamp255(g_ip_octets[g_ip_cursor] - 1);

        draw_ip_editor();
        build_ip(ip,sizeof(ip),g_ip_octets);

        if (kDown & HidNpadButton_A)
        {
            consoleUpdate(NULL);
            break;
        }
        consoleUpdate(NULL);
    }

    client_agent *my_agent = create_client_agent(ip);
    cprintf("client created\n");

    while(appletMainLoop())
    {
        padUpdate(&pad);
        u32 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break;

        if (kDown & HidNpadButton_A) 
        {
            message *m = malloc(sizeof(message));
            m->source = my_agent->uid;
            m->destiny = 0;
            m->index = 0;
            strcpy(m->data, "Hello from Switch\n");

            queue_enqueue(my_agent->to_send_non_reliable, m);
        }
        update_client_agent(my_agent);

        consoleUpdate(NULL);
    }

    socketExit();
    consoleExit(NULL);
    return 0;
}
