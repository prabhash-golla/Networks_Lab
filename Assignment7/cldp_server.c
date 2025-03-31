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
#include <sys/utsname.h>
#include <netdb.h>
#include <pthread.h>

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
void send_packet(int sockfd,uint32_t source_ip,uint32_t dest_ip, uint8_t msg_type, uint16_t transaction_id,const char *payload, int payload_len)
{
    char sendbuf[1500];
    for(int i=0;i<1500;i++) sendbuf[i]='\0';

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

    if (payload_len > 0 && payload != NULL) memcpy(sendbuf + sizeof(struct iphdr) + sizeof(struct cldp_header),payload, payload_len);

    struct sockaddr_in dest;
    dest.sin_family      = AF_INET;
    dest.sin_addr.s_addr = dest_ip;

    if (sendto(sockfd, sendbuf, packet_len, 0, (struct sockaddr *)&dest,sizeof(dest)) < 0) perror("sendto failed");
}

int sockfd;
uint32_t source_ip;

void *hello_sender(void *arg) 
{
    while (1) 
    {
        uint16_t transaction_id = rand() % 65535;
        printf("Sending HELLO message\n");
        uint32_t broadcast_ip = inet_addr("255.255.255.255");
        send_packet(sockfd,source_ip,broadcast_ip, HELLO, transaction_id, NULL, 0);
        sleep(10);
    }
    return NULL;
}

int main(int argc, char *argv[]) 
{
    srand(time(NULL));
    if(argc>1) source_ip  = inet_addr(argv[1]);
    else source_ip = inet_addr("127.0.0.1");

    sockfd = socket(AF_INET, SOCK_RAW, PROTO_CLDP);
    if(sockfd<0)
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

    pthread_t hello_thread;
    if (pthread_create(&hello_thread, NULL, hello_sender, NULL) < 0) 
    {
        perror("Could not create HELLO sender thread");
        exit(EXIT_FAILURE);
    }

    while (1) 
    {
        char buffer[1500];
        memset(buffer, 0, sizeof(buffer));
        struct sockaddr_in src_addr;
        socklen_t addr_len = sizeof(src_addr);
        int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0,(struct sockaddr *)&src_addr, &addr_len);
        if (bytes_received < 0) 
        {
            perror("recvfrom failed");
            continue;
        }

        struct iphdr *ip = (struct iphdr *)buffer;
        if (ip->protocol != PROTO_CLDP) continue;
        // if (ip->check!=compute_checksum((unsigned short *)ip, sizeof(struct iphdr))) continue;

        struct cldp_header *cldp = (struct cldp_header *)(buffer + sizeof(struct iphdr));
        if (cldp->msg_type == QUERY) 
        {
            uint16_t transaction_id = ntohs(cldp->transaction_id);
            printf("Received QUERY from %s, transaction id: %u\n",inet_ntoa(*(struct in_addr *)&ip->saddr), transaction_id);

            char query_payload[512] = {0};
            int query_payload_len = cldp->payload_len;
            if (query_payload_len > 0 && query_payload_len < (int)sizeof(query_payload)) 
            {
                memcpy(query_payload, (char *)((char *)cldp + sizeof(struct cldp_header)), query_payload_len);
                query_payload[query_payload_len] = '\0';
                printf("Query payload: %s\n", query_payload);
            }
            else strcpy(query_payload, "HTS");
            char hostname[256];
            if (gethostname(hostname, sizeof(hostname)) < 0) 
            {
                perror("gethostname failed");
                strcpy(hostname, "unknown");
            }

            struct timeval tv;
            if(gettimeofday(&tv, NULL) < 0) 
            {
                perror("gettimeofday failed");
                tv.tv_sec = 0;
                tv.tv_usec = 0;
            }
            char timestamp[64];
            snprintf(timestamp, sizeof(timestamp), "%ld.%06ld", tv.tv_sec, tv.tv_usec);

            struct utsname uts;
            int uname_success = (uname(&uts) == 0);

            char payload[1024];
            payload[0] = '\0';
            int payload_len = 0;

            if (strchr(query_payload, 'H') || strchr(query_payload, 'h')) payload_len += snprintf(payload + payload_len, sizeof(payload) - payload_len,"Hostname: %s\n", hostname);
            if (strchr(query_payload, 'T') || strchr(query_payload, 't')) payload_len += snprintf(payload + payload_len, sizeof(payload) - payload_len,"Timestamp: %s\n", timestamp);
            if (strchr(query_payload, 'S') || strchr(query_payload, 's')) 
            {
                if (uname_success) payload_len += snprintf(payload + payload_len, sizeof(payload) - payload_len,"Sysname: %s\nRelease: %s\nVersion: %s\nMachine: %s\n",uts.sysname, uts.release, uts.version, uts.machine);
                else payload_len += snprintf(payload + payload_len, sizeof(payload) - payload_len,"System Info: unavailable\n");
            }

            if (payload_len == 0) 
            {
                if (uname_success) payload_len = snprintf(payload, sizeof(payload),"Hostname: %s\nTimestamp: %s\nSysname: %s\nRelease: %s\nVersion: %s\nMachine: %s\n",hostname, timestamp, uts.sysname, uts.release, uts.version, uts.machine);
                else payload_len = snprintf(payload, sizeof(payload),"Hostname: %s\nTimestamp: %s\n",hostname, timestamp);
            }

            uint32_t dest_ip = ip->saddr;
            send_packet(sockfd, source_ip, dest_ip, RESPONSE, transaction_id, payload, payload_len);
            printf("Sent RESPONSE to %s\n",inet_ntoa(*(struct in_addr *)&ip->saddr));

        }
    }

    close(sockfd);
    return 0;
}
