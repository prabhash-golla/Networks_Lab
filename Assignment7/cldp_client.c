#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <time.h>
#include <sys/time.h>
#include <netdb.h>

// ------ Global Variables ------ //
#define PROTO_CLDP 253
#define HELLO 0x01
#define QUERY 0x02
#define RESPONSE 0x03

// ------ CLDP Header ------ //
struct cldp_header 
{
    uint8_t  msg_type;
    uint8_t  payload_len;
    uint16_t transaction_id;
    uint32_t reserved;
} __attribute__((packed));

// ------ COMPUTE CHECKSUM ------ //
unsigned short compute_checksum(unsigned short *buf, int len) 
{
    unsigned long sum = 0;
    while (len > 1) 
    {
        sum += *buf++;
        len -= 2;
    }
    if (len > 0) sum += *(unsigned char *)buf;
    while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);
    return (unsigned short)(~sum);
}

// ------ SEND CLTP PACKET FUNCITION ------ //
void send_packet(int sockfd, uint32_t source_ip, uint32_t dest_ip, uint8_t msg_type, uint16_t transaction_id, const char *payload, int payload_len)
{
    char sendbuf[1500];
    memset(sendbuf, 0, sizeof(sendbuf));

    struct iphdr *ip = (struct iphdr *)sendbuf;
    struct cldp_header *cldp = (struct cldp_header *)(sendbuf + sizeof(struct iphdr));

    int total_payload = sizeof(struct cldp_header) + payload_len;
    int packet_len = sizeof(struct iphdr) + total_payload;
    
    ip->ihl      = 5;
    ip->version  = 4;
    ip->tos      = 0;
    ip->tot_len  = htons(packet_len);
    ip->id       = htons(rand() % 65535);
    ip->frag_off = 0;
    ip->ttl      = 64;
    ip->protocol = PROTO_CLDP;  
    ip->saddr    = source_ip;
    ip->daddr    = dest_ip;
    ip->check    = 0;
    ip->check    = compute_checksum((unsigned short *)ip, sizeof(struct iphdr));

    cldp->msg_type       = msg_type;
    cldp->payload_len    = payload_len;
    cldp->transaction_id = htons(transaction_id);
    cldp->reserved       = 0;

    if (payload_len > 0 && payload != NULL) memcpy(sendbuf + sizeof(struct iphdr) + sizeof(struct cldp_header), payload, payload_len);

    struct sockaddr_in dest_addr;
    dest_addr.sin_family      = AF_INET;
    dest_addr.sin_addr.s_addr = dest_ip;

    if (sendto(sockfd, sendbuf, packet_len, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) perror("sendto failed");
}

// ------ HOST NODE FOR LL(LINKED LIST) ------ //
struct host_node 
{
    uint32_t ip;
    struct host_node *next;
};

// ------ CHECK SERVER IP ALREADY EXISTS IN THE CURRENT LL ------ //
int ip_exists(struct host_node *head, uint32_t ip) 
{
    while(head) 
    {
        if(head->ip == ip) return 1;
        head = head->next;
    }
    return 0;
}

// ------ APPEND THE SERVER IP TO LL ------ //
struct host_node* append_ip(struct host_node *head, uint32_t ip) 
{
    struct host_node *new_node = malloc(sizeof(struct host_node));
    if (!new_node) 
    {
        perror("malloc failed");
        exit(EXIT_FAILURE);
    }
    new_node->ip = ip;
    new_node->next = NULL;
    if (!head) return new_node;
    struct host_node *current = head;
    while(current->next) current = current->next;
    current->next = new_node;
    return head;
}

// ------ FREE LL  ------ //
void free_list(struct host_node *head) 
{
    struct host_node *tmp;
    while (head) 
    {
        tmp = head;
        head = head->next;
        free(tmp);
    }
}

#include <sys/ioctl.h>
void print_terminal_line(char character) 
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    
    char line[w.ws_col + 1];
    memset(line, character, w.ws_col);
    line[w.ws_col] = '\0';
    
    printf("%s\n", line);
}


// ------ MAIN FUNCITION ------ //
int main(int argc, char *argv[]) 
{
    srand(time(NULL));
    int sockfd;
    int k = 7;
    uint32_t source_ip;
    if(argc > 1) source_ip = inet_addr(argv[1]);
    else source_ip = inet_addr("127.0.0.1");

    if(argc > 2) k = atoi(argv[2]);
    if(k>6) k = 6;

    const char *combinations[] = {
        "H",    // Only H
        "T",    // Only T
        "S",    // Only S
        "HT",   // H and T
        "HS",   // H and S
        "TS",   // T ans S
        "HTS"   // H, T, and S
    };

    // ------ RAW SOCKET CREATION ------ //
    sockfd = socket(AF_INET, SOCK_RAW, PROTO_CLDP);
    if (sockfd < 0) 
    {
        perror("SOCKET CREATION ERROR");
        exit(EXIT_FAILURE);
    }

    int one = 1;
    
    int setsock = setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));
    if(setsock<0)
    {
        perror("FAIL TO SET IP_HDRINCL");
        exit(EXIT_FAILURE);
    }

    setsock = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one));
    if(setsock<0)
    {
        perror("FAIL TO SET SO_BROADCAST");
        exit(EXIT_FAILURE);
    }

    struct host_node *host_list = NULL;

    // ------ RECIVE HELLO MESSAGE FOR 10 SEC ------ //
    printf("Waiting for HELLO messages for 10 seconds...\n");
    time_t start_time = time(NULL);
    while(time(NULL) - start_time < 10) 
    {
        char recvbuf[1500];
        memset(recvbuf, 0, sizeof(recvbuf));
        struct sockaddr_in src_addr;
        socklen_t addr_len = sizeof(src_addr);

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

        int bytes_received = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&src_addr, &addr_len);
        if (bytes_received < 0) continue;
        
        struct iphdr *ip = (struct iphdr *)recvbuf;
        if (ip->protocol != PROTO_CLDP) continue;

        struct cldp_header *cldp = (struct cldp_header *)(recvbuf + sizeof(struct iphdr));
        if (cldp->msg_type == HELLO) 
        {
            if (!ip_exists(host_list, ip->saddr)) 
            {
                host_list = append_ip(host_list, ip->saddr);
                printf("Received HELLO from %s\n", inet_ntoa(*(struct in_addr *)&ip->saddr));
            }
        }
    }

    // ------ EXIT IF NO SERVER SENDS HELLO ------ //
    if (!host_list) 
    {
        printf("No HELLO messages received. Exiting.\n");
        close(sockfd);
        return 0;
    }

    // ------ SEND QUERY TO EVERY SERVER ------ //
    struct host_node *current = host_list;
    uint16_t transaction_id = getpid();
    while (current) 
    {
        const char *query_payload = combinations[k];
        int payload_len = strlen(query_payload);
        printf("Sending QUERY to all servers at \"%s\" with Transaction \"ID\" %u and PayLoad %s\n", inet_ntoa(*(struct in_addr *)&current->ip), transaction_id,query_payload);
        send_packet(sockfd, source_ip, current->ip, QUERY, transaction_id, query_payload, payload_len);
        current = current->next;
    }

    struct timeval timeout;
    timeout.tv_sec  = 5;
    timeout.tv_usec = 0;
    
    setsock = setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));
    if(setsock<0)
    {
        perror("FAIL TO SET SO_RCVTIMEO");
        exit(EXIT_FAILURE);
    }

    // ------ RECIVE RESPONSES FROM THE SERVERS ------ //
    printf("Waiting for responses...\n");
    time_t last_recv = time(NULL);
    while (last_recv-time(NULL)<5) 
    {
        char recvbuf[1500];
        memset(recvbuf, 0, sizeof(recvbuf));
        struct sockaddr_in resp_addr;
        socklen_t addr_len = sizeof(resp_addr);
        int bytes_received = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&resp_addr, &addr_len);
        if (bytes_received < 0) break;

        struct iphdr *ip = (struct iphdr *)recvbuf;
        if (ip->protocol != PROTO_CLDP) continue;

        struct cldp_header *cldp = (struct cldp_header *)(recvbuf + sizeof(struct iphdr));
        if (cldp->msg_type == RESPONSE && cldp->transaction_id==htons(transaction_id)) 
        {
            int resp_payload_len = cldp->payload_len;
            char resp_payload[1024];
            if (resp_payload_len > 0 && resp_payload_len < (int)sizeof(resp_payload)) 
            {
                memcpy(resp_payload, recvbuf + sizeof(struct iphdr) + sizeof(struct cldp_header), resp_payload_len);
                resp_payload[resp_payload_len] = '\0';
            } 
            else strcpy(resp_payload, "No payload");
            print_terminal_line('_');
            printf("\nReceived RESPONSE from %s:\n%s", inet_ntoa(*(struct in_addr *)&ip->saddr), resp_payload);
            last_recv = time(NULL);
        }
    }

    print_terminal_line('_');
    free_list(host_list);
    close(sockfd);
    return 0;
}
