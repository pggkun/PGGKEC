#ifndef __PGGKEC__H
#define __PGGKEC__H
/*
MIT License

Copyright (c) 2025 P. G. G. Costa

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>

#include <sys/types.h> 
#include <fcntl.h>
#include <unistd.h>

#if defined(__vita__)
    #define printf psvDebugScreenPrintf
#elif defined(__psp__)
    #define printf pspDebugScreenPrintf
#endif

#ifdef VERBOSE
    #define PGGKEC_LOG(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
    #define PGGKEC_LOG(fmt, ...) ((void)0)
#endif

#if defined(_WIN32) || defined(_WIN64)
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x0601
    #endif
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

    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define closeSocket closesocket
    typedef SOCKET socket_t;
    #define SOCKLEN_T int
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

    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define closeSocket close
    typedef int socket_t;
    #define SOCKLEN_T socklen_t
#endif

#define DATA_BUFFER_SIZE 256

#define DISCOVERY_MSG "DSMSG"
#define DISCOVERY_RESP "DSRES"
#define CONNECTION_MSG "CNMSG"
#define DISCONNECTION_MSG "DNMSG"
#define REGULAR_MSG "RGMSG"

typedef struct message
{
    uint8_t source;
    uint8_t destiny; //if client: 0 = Server, if server: 0 = ALL
    uint8_t index; //0 = not reliable
    char data[DATA_BUFFER_SIZE];
} message;

//==========================================
// Queue
//==========================================
#define advance(i) (i = (i + 1) % q->max)

typedef void* queue_item;

typedef struct queue 
{
    int max;
    int total;
    int front;
    int rear;
    queue_item *items;
} *queue_t;

queue_t queue_create(int m) 
{
    queue_t q = malloc(sizeof(struct queue));
    q->max   = m;
    q->total = 0;
    q->front = 0;
    q->rear  = 0; 
    q->items = malloc(m * sizeof(queue_item));
    return q;
}

int queue_is_empty(queue_t q) 
{
    return (q->total == 0);
}

int queue_is_full(queue_t q) 
{
    return (q->total == q->max);
}

int queue_size(queue_t q) 
{
    return q->total;
}

void queue_enqueue(queue_t q, queue_item x) 
{
    if (queue_is_full(q)) { PGGKEC_LOG("queue full!"); abort(); }
    q->items[q->rear] = x;
    advance(q->rear);
    q->total++;
}

queue_item queue_dequeue(queue_t q) 
{
    if (queue_is_empty(q)) { PGGKEC_LOG("queue empty!"); abort(); }
    queue_item x = q->items[q->front];
    advance(q->front);
    q->total--;
    return x;
}

queue_item queue_front(queue_t q)
{
    if (queue_is_empty(q)) { PGGKEC_LOG("queue empty!"); abort(); }
    queue_item x = q->items[q->front];
    return x;
}

void queue_destroy(queue_t *q_ptr) 
{
    free((*q_ptr)->items);
    free(*q_ptr);
    *q_ptr = NULL;
}
//==========================================
// VECTOR
//==========================================
typedef void* vector_item;

typedef struct vector 
{
    int max;          
    int total;        
    vector_item *items;
} *vector_t;

static inline void vector_ensure_capacity(vector_t v, int min_cap) 
{
    if (v->max >= min_cap) return;

    int new_cap = v->max > 0 ? v->max : 1;
    while (new_cap < min_cap) {
        new_cap = new_cap + (new_cap >> 1) + 1;
        if (new_cap < 0) { PGGKEC_LOG("capacity overflow!"); abort(); }
    }

    vector_item *new_items = realloc(v->items, (size_t)new_cap * sizeof(vector_item));
    if (!new_items) { PGGKEC_LOG("realloc failed!"); abort(); }

    v->items = new_items;
    v->max = new_cap;
}

vector_t vector_create(int initial_capacity) 
{
    if (initial_capacity < 0) { PGGKEC_LOG("negative capacity!"); abort(); }

    vector_t v = malloc(sizeof(struct vector));
    if (!v) { PGGKEC_LOG("malloc failed!"); abort(); }

    v->max   = initial_capacity > 0 ? initial_capacity : 0;
    v->total = 0;
    v->items = v->max ? malloc((size_t)v->max * sizeof(vector_item)) : NULL;

    if (v->max && !v->items) { PGGKEC_LOG("malloc failed!"); abort(); }
    return v;
}

void vector_destroy(vector_t *v_ptr) 
{
    if (!v_ptr || !*v_ptr) return;
    free((*v_ptr)->items);
    free(*v_ptr);
    *v_ptr = NULL;
}

int vector_is_empty(vector_t v) 
{
    return (v->total == 0);
}

int vector_size(vector_t v)
 {
    return v->total;
}

int vector_capacity(vector_t v) 
{
    return v->max;
}

void vector_reserve(vector_t v, int new_capacity) 
{
    if (new_capacity < 0) { PGGKEC_LOG("negative capacity!"); abort(); }
    vector_ensure_capacity(v, new_capacity);
}

void vector_shrink_to_fit(vector_t v) 
{
    if (v->total == v->max) return;
    if (v->total == 0) {
        free(v->items);
        v->items = NULL;
        v->max = 0;
        return;
    }
    vector_item *new_items = realloc(v->items, (size_t)v->total * sizeof(vector_item));
    if (!new_items) { PGGKEC_LOG("realloc failed!"); abort(); }
    v->items = new_items;
    v->max = v->total;
}

void vector_clear(vector_t v) 
{
    v->total = 0;
}

void vector_push_back(vector_t v, vector_item x)
{
    vector_ensure_capacity(v, v->total + 1);
    v->items[v->total++] = x;
}

vector_item vector_pop_back(vector_t v) 
{
    if (vector_is_empty(v)) { PGGKEC_LOG("vector empty!"); abort(); }
    return v->items[--v->total];
}

vector_item vector_get(vector_t v, int index) 
{
    if (index < 0 || index >= v->total) { PGGKEC_LOG("vector_get error: index out of bounds!"); abort(); }
    return v->items[index];
}

void vector_set(vector_t v, int index, vector_item x) 
{
    if (index < 0 || index >= v->total) { PGGKEC_LOG("vector_set error: index out of bounds!"); abort(); }
    v->items[index] = x;
}

void vector_insert(vector_t v, int index, vector_item x) 
{
    if (index < 0 || index > v->total) { PGGKEC_LOG("vector_insert error: index out of bounds!"); abort(); }
    vector_ensure_capacity(v, v->total + 1);

    memmove(&v->items[index + 1], &v->items[index],
            (size_t)(v->total - index) * sizeof(vector_item));

    v->items[index] = x;
    v->total++;
}

vector_item vector_remove_at(vector_t v, int index) 
{
    if (index < 0 || index >= v->total) { PGGKEC_LOG("vector_remove_at error: index out of bounds!"); abort(); }
    vector_item removed = v->items[index];

    memmove(&v->items[index], &v->items[index + 1],
            (size_t)(v->total - index - 1) * sizeof(vector_item));

    v->total--;
    return removed;
}
//===========================

static uint8_t g_recvbuf[sizeof(message)];
static uint8_t g_sendbuf[sizeof(message)];

message * parse_message(const char *buffer)
{
    message *result = malloc(sizeof(message));
    memcpy(result, buffer, sizeof(message));
    return result;
}

void fill_buffer(const message *msg, char *buffer)
{
    memcpy(buffer, msg, sizeof(message));
}

void print_message(const message * msg)
{
    PGGKEC_LOG("src: %d, destiny: %d, index: %d, data=\"%s\"\n", msg->source, msg->destiny, msg->index, msg->data);
}

#define PORT 9999

char buffer[DATA_BUFFER_SIZE];
int listenfd, len;
struct sockaddr_in servaddr, cliaddr;

typedef struct connection
{
    uint8_t id;
    uint8_t message_index;
    struct sockaddr_in addr;
    char device[12];
} connection;

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
    PGGKEC_LOG("IP: %s\n", ip);

    bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
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
        PGGKEC_LOG("error on connect to server\n");
    }
    len = sizeof(cliaddr);
    return sockfd;
}

int receive(socket_t *sockfd, char *buffer)
{
    memset(buffer, 0,sizeof(message));
    int received =  recvfrom(*sockfd, buffer, sizeof(message), 0,
         (struct sockaddr*)&cliaddr, (socklen_t*)&len);
    if(received < 0)
        perror("error receiving: ");
    return received;
}

int receive_from(socket_t *sockfd, char *buffer, struct sockaddr* caddr, socklen_t* addrlen)
{
    memset(buffer, 0,sizeof(message));
    int received =  recvfrom((int)*sockfd, buffer, sizeof(message), 0,
         (struct sockaddr*)caddr, (socklen_t*)addrlen);
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

int after_receive_as_server(socket_t *sockfd,
                            vector_t *connections,
                            char *buffer,
                            struct sockaddr_in *temp_addr)
{
    int already_connected = 0;
    message *received_message = parse_message(buffer);
    if (strcmp(received_message->data, DISCOVERY_MSG) == 0) {
        message disc;
        disc.source = 0;
        disc.destiny = 0;
        disc.index  = 0;
        strcpy(disc.data, DISCOVERY_RESP);

        char disc_response[sizeof(message)];
        memcpy(disc_response, &disc, sizeof(message));

        PGGKEC_LOG("echo to '%s'\n", inet_ntoa(temp_addr->sin_addr));
        send_to(sockfd, (struct sockaddr*)temp_addr, disc_response);

        free(received_message);
        return 1;
    }
    free(received_message);

    for (size_t i = 0; i < vector_size(*connections); i++) {
        const connection *conn = vector_get(*connections, i);
        if (temp_addr->sin_addr.s_addr == conn->addr.sin_addr.s_addr &&
            temp_addr->sin_port        == conn->addr.sin_port)
        {
            
            already_connected = 1;
            break;
        }
    }
    if (!already_connected) {
        uint8_t new_id = (uint8_t)(vector_size(*connections) + 1);

        connection* new_conn = malloc(sizeof(connection));
        new_conn->id = new_id;
        new_conn->message_index = 1;
        new_conn->addr = *temp_addr;
        strcpy(new_conn->device, "UNKNOWN");

        vector_push_back(*connections, new_conn);

        const char *nip = inet_ntoa(temp_addr->sin_addr);
        PGGKEC_LOG("New client connected: %s, id: %u\n", nip, new_id);

        message resp;
        resp.source = 0;
        resp.destiny = new_id;
        resp.index = 0;
        strcpy(resp.data, CONNECTION_MSG);

        char response[sizeof(message)];
        memcpy(response, &resp, sizeof(message));

        send_to(sockfd, (struct sockaddr*)temp_addr, response);
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

mutex_t mutex;
mutex_t received_messages_mutex;

THREAD_FUNC_RETURN receive_messages_as_server(void *agent_addr);
THREAD_FUNC_RETURN receive_messages_as_client(void *agent_addr);

typedef struct server_agent
{
    uint8_t uid;
    queue_t received_messages;
    queue_t to_send_acks;
    vector_t to_send_messages;

    vector_t connections;

    uint32_t total_message_received;
    uint32_t total_ack_sent;
    uint32_t total_message_sent;

    uint8_t message_index;

    socket_t m_sockfd;
    int connected;

    thread_t  listen_thread;
    void * listen_thread_res;
    int listen_thread_status;

    void (*message_callback)(void *, void *);
} server_agent;

typedef struct client_agent
{
    uint8_t uid;
    queue_t received_messages;
    queue_t to_send_acks;
    queue_t to_send_non_reliable;
    vector_t to_send_messages;

    uint32_t total_message_received;
    uint32_t total_ack_sent;
    uint32_t total_message_sent;

    uint8_t message_index;

    socket_t m_sockfd;
    int connected;

    thread_t  listen_thread;
    void * listen_thread_res;
    int listen_thread_status;

    fd_set read_fds, write_fds;
    struct timeval timeout;

    void (*message_callback)(void *, void *);
} client_agent;

void set_client_callback(client_agent* agent, void *func)
{
    agent->message_callback = func;
}

void set_server_callback(server_agent* agent, void *func)
{
    agent->message_callback = func;
}

THREAD_FUNC_RETURN receive_messages_as_server(void *agent_addr) 
{
    server_agent *m_agent = (server_agent *)(agent_addr);
    while (1) {
        memset(g_recvbuf, 0, sizeof(message));
        struct sockaddr_in temp_addr;
        socklen_t addrlen = sizeof(temp_addr);
        int recv = receive_from(&m_agent->m_sockfd, g_recvbuf, (struct sockaddr *)&temp_addr, &addrlen);
        if (recv > 0) 
        {
            MUTEX_LOCK(&received_messages_mutex);
            int result = after_receive_as_server(&m_agent->m_sockfd, &(m_agent->connections), g_recvbuf, &temp_addr);
            if (result != 0) {
                message *m = malloc(sizeof(message));
                memcpy(m, g_recvbuf, sizeof(message));
                if(!queue_is_full(m_agent->received_messages))
                    queue_enqueue(m_agent->received_messages, m);
            }
            MUTEX_UNLOCK(&received_messages_mutex);
        }
    }
}

client_agent *create_client_agent(const char *ip)
{
    client_agent *ca = malloc(sizeof(client_agent));

    ca->total_message_received = 0;
    ca->total_ack_sent = 0;
    ca->total_message_sent = 0;
    ca->message_index = 0;

    ca->m_sockfd = create_client_socket(ip);
    ca->connected = 0;

    ca->received_messages = queue_create(64);
    ca->to_send_acks = queue_create(64);
    ca->to_send_non_reliable = queue_create(64);
    ca->to_send_messages = vector_create(64);

    PGGKEC_LOG("Pointing to: %s\n", ip);
    return ca;
}

void client_send_message(client_agent *m_agent, const message *m)
{
    FD_ZERO(&m_agent->write_fds);
    FD_SET(m_agent->m_sockfd, &m_agent->write_fds);
    if (m->index == 0)
    {
        PGGKEC_LOG("msg sent [NO-RELIABLE]: ");
        print_message(m);

        char buffer[sizeof(message)] = {0};
        memcpy(buffer, &m, sizeof(message));
        send_as_client(m_agent->m_sockfd, buffer);
        m_agent->total_message_sent++;
        return;                
    }
    else
    {
        m_agent->message_index++;
        vector_push_back(m_agent->to_send_messages, (void *) m);
        PGGKEC_LOG("sending r-message: ");
        print_message(m);
    }
}

void client_send_ack(client_agent *m_agent, const message *m)
{
    message *ack = (message*)malloc(sizeof(message));
    memset(ack->data, 0, sizeof(DATA_BUFFER_SIZE));

    ack->source = m->source;
    ack->destiny = m->destiny;
    ack->index = m->index;
    strcpy(ack->data, "ack");
    queue_enqueue(m_agent->to_send_acks, ack);
}

void client_execute_message(client_agent *m_agent, const message *m)
{
    if(strcmp(m->data, CONNECTION_MSG) == 0)
    {
        m_agent->uid = m->destiny;
        PGGKEC_LOG("Conected! using UID: %d\n", m_agent->uid);
    }
    else if(strcmp(m->data, DISCOVERY_MSG) == 0)
    {
        PGGKEC_LOG("Server found!\n");
    }
}

void client_receive_message(client_agent *m_agent, message *m)
{
    if(&m == NULL)
    {
        PGGKEC_LOG("invalid message\n");
        return;
    }
    
    if(strcmp(m->data, "ack") != 0 && m->index == 0)
    {
        PGGKEC_LOG("received: ");
        print_message(m);
    }

    if(strcmp(m->data, "ack") != 0)
    {
        m_agent->message_callback(m_agent, m);
        client_execute_message(m_agent, m);
        if(m->index != 0)
            client_send_ack(m_agent, m);
    }

    if(strcmp(m->data, "ack") == 0)
    {                
        if(m->source == m_agent->uid)
        {
            uint8_t index_at_vector = 0;
            uint8_t found_message = 0;
            for(int i=0; i<m_agent->to_send_messages->total; i++)
            {
                if(m->index == ((message *)vector_get(m_agent->to_send_messages,i))->index)
                {
                    index_at_vector = i;
                    found_message = 1;
                }
            }

            if(!found_message) return;

            void * removed = vector_remove_at(m_agent->to_send_messages, index_at_vector);
            free(removed);
        }
        return;
    }          
}

void update_client_agent(client_agent* m_agent)
{
    FD_ZERO(&m_agent->read_fds);
    FD_ZERO(&m_agent->write_fds);

    FD_SET(m_agent->m_sockfd, &m_agent->read_fds);
    FD_SET(m_agent->m_sockfd, &m_agent->write_fds);

    m_agent->timeout.tv_sec = 0;
    m_agent->timeout.tv_usec = 10;

    int ready = select(m_agent->m_sockfd + 1, &m_agent->read_fds, &m_agent->write_fds, NULL, &m_agent->timeout);
    // if (ready <= 0) return;

    memset(g_recvbuf, 0, sizeof(message));
    if (FD_ISSET(m_agent->m_sockfd, &m_agent->read_fds)) 
    {
        struct sockaddr_storage src;
        memset(&src, 0, sizeof(src));

        SOCKLEN_T srclen = (SOCKLEN_T)sizeof(src);

        int recv = recvfrom(m_agent->m_sockfd, (char*)g_recvbuf, 259, 0, (struct sockaddr*)&src, &srclen);
        if (recv > 0) 
        {
            message *m = malloc(sizeof(message));
            memcpy(m, g_recvbuf, sizeof(message));
            queue_enqueue(m_agent->received_messages, m);
        } else if (recv == 0) {
            PGGKEC_LOG("Conection closed.\n");
        } else {
            PGGKEC_LOG("sock: %d\n", m_agent->m_sockfd);
            perror("Error receiving data");
            return;
        }
    }
    
    while(!queue_is_empty(m_agent->received_messages))
    {
        char message_buffer[sizeof(message)] = {0};
        memcpy(message_buffer, queue_front(m_agent->received_messages), sizeof(message));
        message *m = parse_message(message_buffer);

        client_receive_message(m_agent, m);
        queue_dequeue(m_agent->received_messages);
        free(m);
        m_agent->total_message_received++;
    }

    while(!queue_is_empty(m_agent->to_send_acks))
    {
        queue_dequeue(m_agent->to_send_acks);
        m_agent->total_ack_sent++;
    }

    while(!queue_is_empty(m_agent->to_send_non_reliable))
    {
        if (FD_ISSET(m_agent->m_sockfd, &m_agent->write_fds))
        {
            char buffer[sizeof(message)] = {0};
            message *m = queue_dequeue(m_agent->to_send_non_reliable);

            PGGKEC_LOG("msg sent [NO-RELIABLE]: ");
            print_message(m);

            memcpy(buffer, m, sizeof(message));
            send_as_client(m_agent->m_sockfd, buffer);
            free(m);
        }
    }

    if(vector_is_empty(m_agent->to_send_messages)) return;


    for(int i=0; i<m_agent->to_send_messages->total; i++)
    {
        if (FD_ISSET(m_agent->m_sockfd, &m_agent->write_fds))
        {
            char buffer[sizeof(message)] = {0};
            memcpy(buffer, vector_get(m_agent->to_send_messages,i), sizeof(message));
            send_as_client(m_agent->m_sockfd, buffer);
            m_agent->total_message_sent++;
        }
    }
}

void destroy_client_agent(client_agent *ca)
{
    ca->connected = 0;

    while (!queue_is_empty(ca->received_messages))
    {
        void *p = queue_dequeue(ca->received_messages);
        free(p);
    }
    queue_destroy(&ca->received_messages);

    while (!queue_is_empty(ca->to_send_acks))
    {
        void *p = queue_dequeue(ca->to_send_acks);
        free(p);
    }
    queue_destroy(&ca->to_send_acks);

    while (!queue_is_empty(ca->to_send_non_reliable))
    {
        void *p = queue_dequeue(ca->to_send_non_reliable);
        free(p);
    }
    queue_destroy(&ca->to_send_non_reliable);

    for (int i = 0; i < ca->to_send_messages->total; i++) 
    {
        void *p = vector_get(ca->to_send_messages, i);
        free(p);
    }
    vector_destroy(&ca->to_send_messages);
    closeSocket(ca->m_sockfd);
}

server_agent *create_server_agent()
{
    server_agent *sa = malloc(sizeof(server_agent));

    sa->total_message_received = 0;
    sa->total_ack_sent = 0;
    sa->total_message_sent = 0;
    sa->message_index = 0;

    sa->m_sockfd = create_server_socket();
    sa->connected = 1;

    sa->connections = vector_create(5);
    sa->received_messages = queue_create(64);
    sa->to_send_acks = queue_create(64);
    sa->to_send_messages = vector_create(64);

    THREAD_CREATE(&sa->listen_thread, receive_messages_as_server, sa);
    PGGKEC_LOG("Listen Thread created successfully!\n");
    MUTEX_INIT(&received_messages_mutex);

    return sa;
}

void server_broadcast_message(server_agent *m_agent, char *buff)
{
    message* m = parse_message(buff);
    uint8_t original_source = m->source;
    if(m->index != 0) 
    {
        free (m);
        return;
    }
    free (m);
    
    for(int i=0; i<m_agent->connections->total; i++)
    {
        if(((connection*)vector_get(m_agent->connections, i))->id != original_source)
        {
            const char* ip = inet_ntoa(((connection*)vector_get(m_agent->connections, i))->addr.sin_addr);
            PGGKEC_LOG("sending message to: %s\n", ip);
            send_to(&(m_agent->m_sockfd), (struct sockaddr*)&(((connection*)vector_get(m_agent->connections, i))->addr), buff);
        }
    }
}

void server_send_ack(server_agent *m_agent, const message *m)
{
    message *ack = (message*)malloc(sizeof(message));
    memset(ack->data, 0, sizeof(DATA_BUFFER_SIZE));

    ack->source = m->source;
    ack->destiny = m->destiny;
    ack->index = m->index;
    strcpy(ack->data, "ack");
    queue_enqueue(m_agent->to_send_acks, ack);
}

void server_send_message(server_agent *m_agent, message *m)
{
    m->source = 0;
    if(m->index == 0)
    {
        PGGKEC_LOG("msg sent [NO-RELIABLE]: ");
        print_message(m);

        char sendbuf[sizeof(message)] = {0};
        memset(sendbuf, 0, sizeof(message));
        memcpy(sendbuf, m, sizeof(message));

        MUTEX_LOCK(&received_messages_mutex);
        server_broadcast_message(m_agent, sendbuf);
        MUTEX_UNLOCK(&received_messages_mutex);
        m_agent->total_message_sent++;
        return;
    }

    for(int i=0; i<m_agent->connections->total; i++)
    {
        if(((connection*)vector_get(m_agent->connections, i))->id 
        == m->destiny)
        {
            ((connection*)vector_get(m_agent->connections, i))->message_index++;
            vector_push_back(m_agent->to_send_messages, m);
            return;
        }
    }
}

void server_receive_message(server_agent *m_agent, message *m)
{
    if(&m == NULL)
    {
        PGGKEC_LOG("invalid message\n");
        return;
    }
    
    if(strcmp(m->data, "ack") != 0 && m->index == 0)
    {
        PGGKEC_LOG("received: ");
        print_message(m);
    }

    if(strcmp(m->data, "ack") != 0)
    {
        if(m->index == 0)
            m_agent->message_callback(m_agent, m);
        else
        {
            server_send_ack(m_agent, m);
            for(int i=0; i<m_agent->connections->total; i++)
            {
                if( ((connection*)vector_get(m_agent->connections, i))->id 
                == m->source )
                {
                    if(((connection*)vector_get(m_agent->connections, i))->message_index
                    < m->index )
                    {
                        ((connection*)vector_get(m_agent->connections, i))->message_index = m->index;
                        m_agent->message_callback(m_agent, m);
                    }
                }
            }
        }
    }

    if(strcmp(m->data, "ack") == 0)
    {                
        if(m->source == m_agent->uid)
        {
            uint8_t index_at_vector = 0;
            uint8_t found_message = 0;
            for(int i=0; i<m_agent->to_send_messages->total; i++)
            {
                if(m->index == ((message *)vector_get(m_agent->to_send_messages,i))->index)
                {
                    index_at_vector = i;
                    found_message = 1;
                }
            }

            if(!found_message) return;

            void * removed = vector_remove_at(m_agent->to_send_messages, index_at_vector);
            free(removed);
        }
        return;
    }          
}

void server_update(server_agent *m_agent)
{
    MUTEX_LOCK(&received_messages_mutex);
    while(!queue_is_empty(m_agent->received_messages))
    {
        memset(&g_sendbuf, 0,sizeof(servaddr));
        memcpy(&g_sendbuf, queue_front(m_agent->received_messages), sizeof(message));
        message *m = parse_message(g_sendbuf);

        server_receive_message(m_agent, m);
        server_broadcast_message(m_agent, g_sendbuf);

        void * result = queue_dequeue(m_agent->received_messages);
        free(result);
        free(m);
        m_agent->total_message_received++;
    }
    MUTEX_UNLOCK(&received_messages_mutex);

    while(!queue_is_empty(m_agent->to_send_acks))
    {
        for(int i=0; i<m_agent->connections->total; i++)
        {
            if(((connection*)vector_get(m_agent->connections, i))->id 
            == ((message*)queue_front(m_agent->to_send_acks))->source)
            {
                char buffer[sizeof(message)] = {0};
                memcpy(buffer, (message*)queue_front(m_agent->to_send_acks), sizeof(message));
                send_to(&m_agent->m_sockfd, (struct sockaddr*)&(((connection*)vector_get(m_agent->connections, i))->addr), buffer);
            }
        }
        void *ack = queue_dequeue(m_agent->to_send_acks);
        free(ack);
        m_agent->total_ack_sent++;
    }

    if(vector_is_empty(m_agent->to_send_messages)) return;

    for(int i=0; i<m_agent->to_send_messages->total; i++)
    {
        char buffer[sizeof(message)] = {0};
        memcpy(buffer, vector_get(m_agent->to_send_messages,i), sizeof(message));
        MUTEX_LOCK(&received_messages_mutex);
        server_broadcast_message(m_agent, buffer);
        MUTEX_UNLOCK(&received_messages_mutex);
        m_agent->total_message_sent++;
    }
}

void destroy_server_agent(server_agent *sa)
{
    sa->connected = 0;
    THREAD_JOIN(&sa->listen_thread);

    while (!queue_is_empty(sa->received_messages))
    {
        void *p = queue_dequeue(sa->received_messages);
        free(p);
    }
    queue_destroy(&sa->received_messages);

    while (!queue_is_empty(sa->to_send_acks))
    {
        void *p = queue_dequeue(sa->to_send_acks);
        free(p);
    }
    queue_destroy(&sa->to_send_acks);

    for (int i = 0; i < sa->to_send_messages->total; i++) 
    {
        void *p = vector_get(sa->to_send_messages, i);
        free(p);
    }
    vector_destroy(&sa->to_send_messages);
    vector_destroy(&sa->connections);
    MUTEX_DESTROY(&received_messages_mutex);
    closeSocket(sa->m_sockfd);
}

THREAD_FUNC_RETURN update_messages_as_server(void *agent_addr)
{
    server_agent *m_agent = (server_agent*)agent_addr;
    char line[DATA_BUFFER_SIZE];

    while(1)
    {
        if (!fgets(line, sizeof(line), stdin)) return 0;

        size_t len = strcspn(line, "\r\n");
        line[len] = '\0';

        if (len == 0) continue;

        message m;
        memset(&m, 0, sizeof(m));

        if (line[0] == 'R') 
        {
            m.index = m_agent->message_index;
            if (m.index == 0) 
            {
                m.index = 1;
                m_agent->message_index++;
            }
        } else 
        {
            m.index = 0;
        }

        strncpy(m.data, line, DATA_BUFFER_SIZE - 1);
        m.data[DATA_BUFFER_SIZE - 1] = '\0';
        m.source = m_agent->uid;

        server_send_message(m_agent, &m);
    }
}

THREAD_FUNC_RETURN update_messages_as_client(void *agent_addr)
{
    client_agent *m_agent = (client_agent*)agent_addr;
    char line[DATA_BUFFER_SIZE];

    while(1)
    {
        if (!fgets(line, sizeof(line), stdin)) return 0;

        size_t len = strcspn(line, "\r\n");
        line[len] = '\0';

        if (len == 0) continue;

        message *m = malloc(sizeof(message));
        memset(m, 0, sizeof(message));

        if (line[0] == 'R') 
        {
            m->index = m_agent->message_index;
            if (m->index == 0) 
            {
                m->index = 2;
                m_agent->message_index = 2;
            }
            m_agent->message_index++;
        } else 
        {
            m->index = 0;
        }

        strncpy(m->data, line, DATA_BUFFER_SIZE - 1);
        m->data[DATA_BUFFER_SIZE - 1] = '\0';
        m->source = m_agent->uid;

        if(m->index == 0)
            queue_enqueue(m_agent->to_send_non_reliable, m);
        else
            vector_push_back(m_agent->to_send_messages, m);
    }
}

void client_agent_enqueue_non_reliable(client_agent* m_agent, uint8_t source, uint8_t destiny, uint8_t index, const char* data)
{
    message *m = malloc(sizeof(message));
    m->source = source;
    m->destiny = destiny;
    m->index = index;
    strcpy(m->data, data);
    queue_enqueue(m_agent->to_send_non_reliable, m);
}

#endif
