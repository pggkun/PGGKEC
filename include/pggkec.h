#ifndef __PGGKEC__H
#define __PGGKEC__H

#include <iostream>
#include <queue>
#include <tuple>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
    #define _WIN32_WINNT 0x0601
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    typedef HANDLE thread_t;
    typedef CRITICAL_SECTION mutex_t;
    #define THREAD_FUNC_RETURN DWORD WINAPI
    #define closeSocket closesocket
    #define THREAD_CREATE(t, func, param) \
        (*(t) = CreateThread(NULL, 0, func, param, 0, NULL))
    #define THREAD_JOIN(t) \
        WaitForSingleObject(*t, INFINITE)
    #define MUTEX_INIT(m) InitializeCriticalSection(m)
    #define MUTEX_DESTROY(m) DeleteCriticalSection(m)
    #define MUTEX_LOCK(m) EnterCriticalSection(m)
    #define MUTEX_UNLOCK(m) LeaveCriticalSection(m)
#elif defined(__3DS__)
    #include <3ds.h>
    typedef Thread thread_t;
    typedef LightLock mutex_t;
    #define STACKSIZE (2 * 1024)
    #define THREAD_FUNC_RETURN void
    #define THREAD_CREATE(t, func, param) \
        (*(t) = threadCreate(func, param, STACKSIZE, 0x3F, -2, false))
    #define THREAD_JOIN(t) \
        threadJoin(*t, U64_MAX)
    #define MUTEX_INIT(m) LightLock_Init(m)
    #define MUTEX_DESTROY(m) free(m)
    #define MUTEX_LOCK(m) LightLock_Lock(m)
    #define MUTEX_UNLOCK(m) LightLock_Unlock(m)
#else
    #include <pthread.h>
    typedef pthread_t thread_t;
    typedef pthread_mutex_t mutex_t;
    #define THREAD_FUNC_RETURN void *
    #define THREAD_CREATE(t, func, param) \
        pthread_create(t, NULL, func, param)
    #define THREAD_JOIN(t) \
        pthread_join(*t, NULL)
    #define MUTEX_INIT(m) pthread_mutex_init(m, NULL)
    #define MUTEX_DESTROY(m) pthread_mutex_destroy(m)
    #define MUTEX_LOCK(m) pthread_mutex_lock(m)
    #define MUTEX_UNLOCK(m) pthread_mutex_unlock(m)
    #define closeSocket close
#endif

#if defined(__vita__)
    #define printf psvDebugScreenPrintf
#elif defined(__psp__)
    #include <pspnet_inet.h>
    #define printf pspDebugScreenPrintf
#endif

#define DATA_BUFFER_SIZE 256

#define DISCOVERY_MSG "DSMSG"
#define DISCOVERY_RESP "DSRES"
#define CONNECTION_MSG "CNMSG"
#define DISCONNECTION_MSG "DNMSG"
#define REGULAR_MSG "RGMSG"

struct message
{
    uint8_t source;
    uint8_t destiny; //if client: 0 = Server, if server: 0 = ALL
    uint8_t index; //0 = not reliable
    char data[DATA_BUFFER_SIZE];
};

static uint8_t g_recvbuf[sizeof(message)];
static uint8_t g_sendbuf[sizeof(message)];

message parse_message(const char *buffer)
{
    message result;
    memcpy(&result, buffer, sizeof(message));
    return result;
}

void fill_buffer(const message &msg, char *buffer)
{
    memcpy(buffer, &msg, sizeof(message));
}

void print_message(const message& msg)
{
    printf("src: %d, destiny: %d, index: %d, data=\"%s\"\n", msg.source, msg.destiny, msg.index, msg.data);
}

//==================================================================================
// SOCKETS
//==================================================================================
#include <stdio.h>
#include <strings.h> 
#include <sys/types.h> 
#include <fcntl.h>
#include <unistd.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define closeSocket closesocket
    typedef SOCKET socket_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define closeSocket close
    typedef int socket_t;
#endif

#define PORT 9999

char buffer[DATA_BUFFER_SIZE];
int listenfd, len;
sockaddr_in servaddr, cliaddr;

struct connection
{
    uint8_t id=0;
    uint8_t message_index=0;
    sockaddr_in addr;
    char device[12] = {0};
};

int send_as_client(socket_t sockfd, char *buffer);

socket_t create_server_socket()
{
    memset(&servaddr, 0,sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    socket_t sockfd;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);         
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_family = AF_INET; 

    const char* ip = inet_ntoa(servaddr.sin_addr);
    printf("IP: %s\n", ip);

    bind(sockfd, (sockaddr*)&servaddr, sizeof(servaddr));
    len = sizeof(cliaddr);

    return sockfd;
}

socket_t create_client_socket(const char *ip)
{
    memset(&servaddr,0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    socket_t sockfd;
    servaddr.sin_addr.s_addr = inet_addr(ip); 
    servaddr.sin_port = htons(PORT); 
    servaddr.sin_family = AF_INET; 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        printf("error on connect to server\n");
    }
    len = sizeof(cliaddr);
    return sockfd;
}

int receive(socket_t *sockfd, char *buffer)
{
    memset(buffer, 0,sizeof(message));
    int received =  recvfrom(*sockfd, buffer, sizeof(message), 0,
         (struct sockaddr*)&cliaddr, (socklen_t*)&len);
    if(received <= 0)
        perror("error receiving: ");
    return received;
}

int receive_from(socket_t *sockfd, char *buffer, sockaddr* caddr, socklen_t* addrlen)
{
    memset(buffer, 0,sizeof(message));
    int received =  recvfrom((int)*sockfd, buffer, sizeof(message), 0,
         (struct sockaddr*)caddr, (socklen_t*)addrlen);
    if(received <= 0)
        perror("error receiving: ");
    return received;
}

int send_as_server(socket_t *sockfd, char *buffer)
{
    return sendto(*sockfd, buffer, sizeof(message), 0, 
          (struct sockaddr*)&cliaddr, sizeof(cliaddr));
}

int send_to(socket_t *sockfd, struct sockaddr* client, char *buffer)
{
    return sendto(*sockfd, buffer, sizeof(message), 0, 
          client, sizeof(*client));
}

//if new connection: return 0, else return 1;
int after_receive_as_server(socket_t *sockfd, std::vector<connection> &connections, char *buffer, sockaddr_in &temp_addr)
{
    // printf("msg received\n");
    bool already_connected = false;

    message received_message = parse_message(buffer);
    if(strcmp(received_message.data, DISCOVERY_MSG) == 0)
    {
        message disc;
        disc.source = 0;
        disc.destiny = 0;
        disc.index = 0;
        strcpy(disc.data, DISCOVERY_RESP);
        char disc_response[sizeof(message)];
        memcpy(disc_response, &disc, sizeof(message));
        printf("echo to '%s'\n", inet_ntoa(temp_addr.sin_addr));
        send_to(sockfd, (struct sockaddr*)&temp_addr, disc_response);
        return 1;
    }

    for(auto conn : connections)
    {
        if((temp_addr.sin_addr.s_addr == conn.addr.sin_addr.s_addr) && (temp_addr.sin_port == conn.addr.sin_port))
        {
            already_connected = true;
            break;
        }
    }
    if(!already_connected)
    {
        uint8_t new_id = connections.size() + 1;
        connection new_conn;
        new_conn.id = new_id;
        new_conn.message_index = 1;
        new_conn.addr = temp_addr;
        strcpy(new_conn.device, "UNKNOWN");

        connections.push_back(new_conn);
        const char* nip = inet_ntoa(temp_addr.sin_addr);
        printf("New client connected: %s, id: %d\n", nip, new_id);

        message resp;
        resp.source = 0;
        resp.destiny = new_id;
        resp.index = 0;
        strcpy(resp.data, CONNECTION_MSG);
        char response[sizeof(message)];
        memcpy(response, &resp, sizeof(message));

        send_to(sockfd, (struct sockaddr*)&temp_addr, response);
        return 0;
    }
    return 1;
}

int send_as_client(socket_t sockfd, char *buffer)
{
    int bytes_sent = sendto(sockfd, buffer, sizeof(message), 0, (struct sockaddr*)&servaddr, sizeof(servaddr)); 
    if (bytes_sent == -1) 
    {
        bytes_sent = send(sockfd, buffer, sizeof(message), 0); 
    }
    return bytes_sent;
}

//==================================================================================
// AGENT
//==================================================================================
mutex_t mutex;
mutex_t received_messages_mutex;

THREAD_FUNC_RETURN receive_messages_as_server(void *agent_addr);
THREAD_FUNC_RETURN receive_messages_as_client(void *agent_addr);

class agent
{
    public:
        uint8_t uid;
        std::queue<message> received_messages;
        std::queue<message> to_send_acks;
        std::vector<message> to_send_messages;

        uint32_t total_message_received = 0;
        uint32_t total_ack_sent = 0;
        uint32_t total_message_sent = 0;

        uint8_t message_index = 0;

        socket_t m_sockfd;
        bool connected = false;

        std::vector<std::tuple<uint8_t, uint8_t>> message_indices;

        thread_t  listen_thread;
        void * listen_thread_res;
        int listen_thread_status;

        agent()
        {
            to_send_messages.clear();
        }

        virtual void default_process_message(const message &m)
        {
            return;
        }
        void (agent::*process_message)(const message&) = &agent::default_process_message;

        void execute_message(const message& m) 
        {
            (this->*process_message)(m);
        }

        void send_ack(const message &m)
        {
            message *ack = (message*)malloc(sizeof(message));
            memset(ack->data, 0, sizeof(DATA_BUFFER_SIZE));

            ack->source = m.source;
            ack->destiny = m.destiny;
            ack->index = m.index;
            strcpy(ack->data, "ack");

            //send (add to ack queue)
            
            // printf("to send ack: ");
            // print_message(*ack);

            to_send_acks.push(*ack);
        }

        virtual void send_message(message &m) {
            printf("send message virtual\n");
        }

        virtual void listen_for_messages()
        {
            char buff[sizeof(message)];
            if(receive(&m_sockfd, buff) > 0)
            {
                message m = parse_message(buff);
                received_messages.push(m);
            }
        }

        void receive_message(message m)
        {
            if(&m == NULL || &m == nullptr)
            {
                printf("invalid message\n");
                return;
            }
            
            if(strcmp(m.data, "ack") != 0 && m.index == 0)
            {
                printf("received: ");
                print_message(m);
            }

            if(strcmp(m.data, "ack") != 0)
            {
                //process_message
                execute_message(m);
                if(m.index != 0)
                    send_ack(m);
            }

            //se receber um ack, excluir msg da lista a ser enviada
            if(strcmp(m.data, "ack") == 0)
            {                
                if(m.source == uid)
                {
                    for (auto it = to_send_messages.begin(); it != to_send_messages.end();) 
                    {
                        if (it->index == m.index) 
                        {
                            it = to_send_messages.erase(it);
                            printf("callback to message: %d\n", m.index);
                            return;
                        } else {
                            ++it;
                        }
                    }
                }
                return;
            }                
        }

        virtual void update()
        {
            return;
        }
};

class server_agent : public agent 
{
    public:
        server_agent()
        {
            m_sockfd = create_server_socket();
            connected = true;

            THREAD_CREATE(&listen_thread, receive_messages_as_server, this);
            printf("Listen Thread criado com sucesso!\n");
            MUTEX_INIT(&received_messages_mutex);
        }

        ~server_agent()
        {
            THREAD_JOIN(&listen_thread);
            printf("Listen Thread finalizado!\n");
        }

        std::vector<sockaddr_in> addrs;
        std::vector<connection> connections;

        void default_process_message(const message &m) override 
        {
        }

        void send_message(message &m) override
        {
            m.source = 0;
            if(m.index == 0)
            {
                printf("msg sent [NO-RELIABLE]: ");
                print_message(m);

                char buffer[sizeof(message)] = {0};
                memcpy(buffer, &m, sizeof(message));
                MUTEX_LOCK(&received_messages_mutex);
                broadcast_message(buffer);
                MUTEX_UNLOCK(&received_messages_mutex);
                total_message_sent++;
                return;
            }

            //não sei se isso funciona, precisa testar reliable messages
            for(auto conn : connections)
            {
                if(conn.id == m.destiny)
                {
                    conn.message_index++;
                    to_send_messages.push_back(m);
                    return;
                }
            }
        }


        void broadcast_message(char *buff)
        {
            message m = parse_message(buff);
            uint8_t original_source = m.source;
            if(m.index != 0) return; //broadcast somente de msg no-reliable
            
            for(auto conn : connections)
            {
                //broadcast para todas as conexões que não são a fonte da msg
                if(conn.id != original_source)
                {
                    const char* ip = inet_ntoa(conn.addr.sin_addr);
                    printf("sending message to: %s\n", ip);
                    send_to(&m_sockfd, (sockaddr*)&conn.addr, buff);
                }
            }
        }

        void update() override
        {
            //listen for new messages
            MUTEX_LOCK(&received_messages_mutex);
            while(!received_messages.empty())
            {
                char message_buffer[sizeof(message)]= {0};
                memcpy(message_buffer, &received_messages.front(), sizeof(message));
                message m = parse_message(message_buffer);

                receive_message(m);
                broadcast_message(message_buffer);
                received_messages.pop();
                total_message_received++;
            }
            MUTEX_UNLOCK(&received_messages_mutex);

            //send acks (TODO: implementar envio dependendo da fonte)
            while(!to_send_acks.empty())
            {
                for(auto conn: connections)
                {
                    if(conn.id == to_send_acks.front().source)
                    {
                        // print_message(to_send_acks.front());
                        char buffer[sizeof(message)] = {0};
                        memcpy(buffer, &to_send_acks.front(), sizeof(message));
                        send_to(&m_sockfd, (sockaddr *)&conn.addr, buffer);

                    }
                }
                to_send_acks.pop();
                total_ack_sent++;
            }

            if(to_send_messages.empty()) return;

            for(auto msg : to_send_messages)
            {
                // printf("msg sent [RELIABLE]: ");
                // print_message(msg);

                char buffer[sizeof(message)] = {0};
                memcpy(buffer, &msg, sizeof(message));
                MUTEX_LOCK(&received_messages_mutex);
                broadcast_message(buffer);
                MUTEX_UNLOCK(&received_messages_mutex);
                total_message_sent++;
            }
        }
};

class client_agent : public agent 
{
    public:
        client_agent(const char *ip)
        {
            m_sockfd = create_client_socket(ip);
        }

        client_agent()
        {
        }

        fd_set read_fds, write_fds;
        struct timeval timeout;

        ~client_agent()
        {
        }

        uint8_t server_message_index = 0;

        void default_process_message(const message &m) override
        {
            //se for mensagem de conexão
            if(strcmp(m.data, CONNECTION_MSG) == 0)
            {
                uid = m.destiny;
                printf("Conexão concluída, utilizando UID: %d\n", uid);
            }
            else if(strcmp(m.data, DISCOVERY_MSG) == 0)
            {
                printf("Servidor encontrado\n");
            }
        }

        void send_connection_request()
        {
            message conn_request = {0, 0, 1, "conn plz"};
        }

        void send_message(message &m) override
        {
            FD_ZERO(&write_fds);
            FD_SET(m_sockfd, &write_fds);
            if (m.index == 0)
            {
                printf("msg sent [NO-RELIABLE]: ");
                print_message(m);

                char buffer[sizeof(message)] = {0};
                memcpy(buffer, &m, sizeof(message));
                send_as_client(m_sockfd, buffer);
                total_message_sent++;
                return;                
            }
            else
            {
                message_index++;
                to_send_messages.push_back(m);
                printf("sending r-message: ");
                print_message(m);
            }
        }

        void update() override 
        {
            FD_ZERO(&read_fds);
            FD_ZERO(&write_fds);

            FD_SET(m_sockfd, &read_fds);
            FD_SET(m_sockfd, &write_fds);

            timeout.tv_sec = 0;
            timeout.tv_usec = 10;

            int ready = select(m_sockfd + 1, &read_fds, &write_fds, NULL, &timeout);
            // if (ready <= 0) return;

            memset(g_recvbuf, 0, sizeof(message));
            if (FD_ISSET(m_sockfd, &read_fds)) 
            {
                sockaddr_storage src{};
                socklen_t srclen = sizeof(src);

                int recv = recvfrom(m_sockfd, g_recvbuf, sizeof(message), 0, reinterpret_cast<sockaddr*>(&src), &srclen);
                if (recv > 0) 
                {
                    message m;
                    memcpy(&m, g_recvbuf, sizeof(message));
                    received_messages.push(m);
                } else if (recv == 0) {
                    printf("Conexão encerrada pelo remetente.\n");
                    // return;
                } else {
                    printf("sock: %d\n", m_sockfd);
                    perror("Erro ao receber dados");
                    return;
                }
            }
            
            while(!received_messages.empty())
            {
                char message_buffer[sizeof(message)] = {0};
                memcpy(message_buffer, &received_messages.front(), sizeof(message));
                message m = parse_message(message_buffer);

                receive_message(m);
                received_messages.pop();
                total_message_received++;
            }

            while(!to_send_acks.empty())
            {
                // printf("ack sent: ");
                // print_message(to_send_acks.front());
                to_send_acks.pop();
                total_ack_sent++;
            }

            if(to_send_messages.empty()) return;

            for(auto msg : to_send_messages)
            {
                if (FD_ISSET(m_sockfd, &write_fds)) 
                {
                    // printf("stack: %d, msg sent [RELIABLE]: ", to_send_messages.size());
                    // print_message(msg);

                    char buffer[sizeof(message)] = {0};
                    memcpy(buffer, &msg, sizeof(message));
                    send_as_client(m_sockfd, buffer);
                    total_message_sent++;
                }
            }
        }
};

std::string get_device_ip()
{
    #if defined(__3DS__)
        sockaddr_in tmp;
        tmp.sin_addr.s_addr = gethostid();
        printf("Device IP: %s\n",inet_ntoa(tmp.sin_addr));
        return std::string(inet_ntoa(tmp.sin_addr));

    #elif defined(__PSP__)
        union SceNetApctlInfo info;
		if (sceNetApctlGetInfo(8, &info) != 0)
			strcpy(info.ip, "");
        printf("Device IP: %s\n",info.ip);
        return std::string(info.ip);

    #elif defined(_WIN32)
        char hostname[256];
        struct addrinfo hints, *res, *ptr;

        if (gethostname(hostname, sizeof(hostname)) == SOCKET_ERROR) {
            printf("Erro ao obter o nome do host: %d\n", WSAGetLastError());
            WSACleanup();
            return "";
        }

        printf("Nome do host: %s\n", hostname);

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM; 
        hints.ai_protocol = IPPROTO_TCP; 

        if (getaddrinfo(hostname, NULL, &hints, &res) != 0) {
            printf("getaddrinfo falhou: %d\n", WSAGetLastError());
            WSACleanup();
            return "";
        }
        std::string ip_address;
        for (ptr = res; ptr != NULL; ptr = ptr->ai_next)
        {
            struct sockaddr_in *sockaddr_ipv4 = (struct sockaddr_in *)ptr->ai_addr;
            ip_address = inet_ntoa(sockaddr_ipv4->sin_addr);
            break;
            // printf("Device IP: %s\n",ip_address.c_str());
        }
        printf("Device IP: %s\n",ip_address.c_str());
        return ip_address;
    #endif
    return "";
}

std::string server_discovery()
{
#ifndef _WIN32
    std::string base_ip = get_device_ip();

    socket_t sockfd;
    struct sockaddr_in svr_addr, cli_addr;
    char buffer[sizeof(message)];
    fd_set read_fds;
    struct timeval timeout;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Erro ao criar o socket");
    }

    memset(&cli_addr, 0, sizeof(cli_addr));
    cli_addr.sin_family = AF_INET;
    cli_addr.sin_addr.s_addr = INADDR_ANY;
    cli_addr.sin_port = htons(PORT);

    if (bind(sockfd, (struct sockaddr*)&cli_addr, sizeof(cli_addr)) < 0) {
        perror("Erro ao fazer bind do socket");
    }

    std::string network_prefix = base_ip.substr(0, base_ip.find_last_of('.') + 1);
    for (int i = 1; i <= 255; ++i) 
    {
        std::string target_ip = network_prefix + std::to_string(i);
        printf("asking: '%s'\n", target_ip.c_str());

        memset(&svr_addr, 0, sizeof(svr_addr));
        svr_addr.sin_family = AF_INET;
        svr_addr.sin_port = htons(PORT);

        if (inet_pton(AF_INET, target_ip.c_str(), &svr_addr.sin_addr) <= 0) {
            perror("Erro ao converter o endereço IP");
            continue;
        }

        message disc;
        disc.source = 0;
        disc.destiny = 0;
        disc.index = 0;
        strcpy(disc.data, DISCOVERY_MSG);
        char disc_message[sizeof(message)];
        memcpy(disc_message, &disc, sizeof(message));

        if (sendto(sockfd, disc_message, sizeof(message), 0,
                   (struct sockaddr*)&svr_addr, sizeof(svr_addr)) < 0) {
            perror("Erro ao enviar mensagem");
            continue;
        }

        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        timeout.tv_sec = 0;
        timeout.tv_usec = 10;

        if (select(sockfd + 1, &read_fds, nullptr, nullptr, &timeout) > 0) {
            socklen_t addr_len = sizeof(svr_addr);
            ssize_t recv_len = recvfrom(sockfd, buffer, sizeof(buffer) - 1, 0,
                                        (struct sockaddr*)&svr_addr, &addr_len);
            if (recv_len > 0) {
                message discr = parse_message(buffer);
                if(strcmp(discr.data, DISCOVERY_RESP)==0)
                {
                    std::string response_ip = inet_ntoa(svr_addr.sin_addr);
                    close(sockfd);
                    return response_ip;
                }
            }
        }
    }

    close(sockfd);
    printf("No server was found\n");
#endif
    return "";
}

THREAD_FUNC_RETURN receive_messages_as_server(void *agent_addr) 
{
    server_agent *m_agent = static_cast<server_agent *>(agent_addr);
    while (true) {
        char buffer[sizeof(message)] = {0};

        sockaddr_in temp_addr;
        socklen_t addrlen = sizeof(temp_addr);
        int recv = receive_from(&m_agent->m_sockfd, buffer, (struct sockaddr *)&temp_addr, &addrlen);
        if (recv > 0) 
        {
            MUTEX_LOCK(&received_messages_mutex);
            int result = after_receive_as_server(&m_agent->m_sockfd, m_agent->connections, buffer, temp_addr);
            if (result != 0) {
                message m;
                memcpy(&m, buffer, sizeof(message));
                m_agent->received_messages.push(m);
            }
            MUTEX_UNLOCK(&received_messages_mutex);
        }
    }
}


//==================================================================================
// INPUT
//==================================================================================

//valido apenas para debug, precisa ser desativado na versão final
THREAD_FUNC_RETURN update_messages(void * agent_addr)
{
    std::string line;

    while(true)
    {
        agent* m_agent = static_cast<agent*>(agent_addr);
        std::getline(std::cin, line);

        if (line.empty()) {
            continue;
        }

        message m;
        memset(&m, 0, sizeof(message));
        memset(&m.data, 0, DATA_BUFFER_SIZE);
        if(line[0] == 'R')
        {
            m.index = m_agent->message_index;
            if(m.index == 0)
            {
                m.index = 1;
                m_agent->message_index++;
            }
        }else
        {
            m.index = 0;
        }

        std::copy(line.begin(), line.end(), m.data);
        m.source = m_agent->uid;
        m_agent->send_message(m);

    }
}

#endif