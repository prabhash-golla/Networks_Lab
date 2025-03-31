/*
 * cldp_client.c
 *
 * A simple CLDP client that:
 *  - Constructs and sends a QUERY message (requesting hostname and timestamp)
 *    to the broadcast address.
 *  - Waits for RESPONSE messages and displays the metadata received.
 *
 * Compile with:
 *   gcc -o cldp_client cldp_client.c
 *
 * Run with sudo:
 *   sudo ./cldp_client [local_ip]
 *   (If no local_ip is provided, "127.0.0.1" is used by default.)
 */

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
 
 #define PROTO_CLDP   253
 #define MSG_HELLO    0x01
 #define MSG_QUERY    0x02
 #define MSG_RESPONSE 0x03
 
 // Custom CLDP header (8 bytes)
 struct cldp_header {
     uint8_t  msg_type;
     uint8_t  payload_len;
     uint16_t transaction_id;
     uint32_t reserved;
 } __attribute__((packed));
 
 // Compute IP header checksum
 unsigned short compute_checksum(unsigned short *buf, int len) {
     unsigned long sum = 0;
     while (len > 1) {
         sum += *buf++;
         len -= 2;
     }
     if (len > 0)
         sum += *(unsigned char *)buf;
     while (sum >> 16)
         sum = (sum & 0xffff) + (sum >> 16);
     return (unsigned short)(~sum);
 }
 
 // Global socket and source IP (network byte order)
 int sockfd;
 uint32_t source_ip;
 
 // Helper function: Build and send an IP packet carrying a CLDP message.
 void send_packet(uint32_t dest_ip, uint8_t msg_type, uint16_t transaction_id,
                  const char *payload, int payload_len)
 {
     char buffer[1500];
     memset(buffer, 0, sizeof(buffer));
 
     struct iphdr *ip = (struct iphdr *)buffer;
     struct cldp_header *cldp = (struct cldp_header *)(buffer + sizeof(struct iphdr));
 
     int total_payload = sizeof(struct cldp_header) + payload_len;
     int packet_len = sizeof(struct iphdr) + total_payload;
 
     /* Fill in the IP header */
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
 
     /* Fill in the CLDP header */
     cldp->msg_type       = msg_type;
     cldp->payload_len    = payload_len;
     cldp->transaction_id = htons(transaction_id);
     cldp->reserved       = 0;
 
     if (payload_len > 0 && payload != NULL)
         memcpy(buffer + sizeof(struct iphdr) + sizeof(struct cldp_header),
                payload, payload_len);
 
     struct sockaddr_in dest;
     dest.sin_family      = AF_INET;
     dest.sin_addr.s_addr = dest_ip;
 
     if (sendto(sockfd, buffer, packet_len, 0, (struct sockaddr *)&dest,
                sizeof(dest)) < 0) {
         perror("sendto failed");
     }
 }
 
 int main(int argc, char *argv[]) {
     srand(time(NULL));
 
     // Optionally, use provided local IP; otherwise default to 127.0.0.1.
     if (argc > 1) {
         source_ip = inet_addr(argv[1]);
     } else {
         source_ip = inet_addr("127.0.0.1");
     }
 
     if ((sockfd = socket(AF_INET, SOCK_RAW, PROTO_CLDP)) < 0) {
         perror("Socket creation failed");
         exit(EXIT_FAILURE);
     }
 
     int one = 1;
     if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
         perror("Failed to set IP_HDRINCL");
         exit(EXIT_FAILURE);
     }
 
     if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one)) < 0) {
         perror("Failed to set SO_BROADCAST");
         exit(EXIT_FAILURE);
     }
 
     // Construct and send a QUERY message.
     uint16_t transaction_id = rand() % 65535;
     // Here, we include a simple payload ("HT") to indicate a request for hostname and timestamp.
     char query_payload[] = "HT";
     int payload_len = strlen(query_payload);
     printf("Sending QUERY message with transaction id %u\n", transaction_id);
     uint32_t broadcast_ip = inet_addr("255.255.255.255");
     send_packet(broadcast_ip, MSG_QUERY, transaction_id, query_payload, payload_len);
 
     // Set a timeout for receiving responses.
     struct timeval timeout;
     timeout.tv_sec  = 5;
     timeout.tv_usec = 0;
     if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0) {
         perror("setsockopt failed");
     }
 
     // Listen for RESPONSE messages.
     while (1) {
         char buffer[1500];
         memset(buffer, 0, sizeof(buffer));
         struct sockaddr_in src_addr;
         socklen_t addr_len = sizeof(src_addr);
         int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                       (struct sockaddr *)&src_addr, &addr_len);
         if (bytes_received < 0) {
             perror("recvfrom failed or timed out");
             break;
         }
 
         struct iphdr *ip = (struct iphdr *)buffer;
         if (ip->protocol != PROTO_CLDP)
             continue;
         struct cldp_header *cldp = (struct cldp_header *)(buffer + sizeof(struct iphdr));
 
         if (cldp->msg_type == MSG_RESPONSE) {
             uint16_t resp_transaction_id = ntohs(cldp->transaction_id);
             if (resp_transaction_id != transaction_id)
                 continue; // Ignore responses that don't match our query
 
             int resp_payload_len = cldp->payload_len;
             char resp_payload[1024];
             if (resp_payload_len > 0 && resp_payload_len < (int)sizeof(resp_payload)) {
                 memcpy(resp_payload,
                        buffer + sizeof(struct iphdr) + sizeof(struct cldp_header),
                        resp_payload_len);
                 resp_payload[resp_payload_len] = '\0';
             } else {
                 strcpy(resp_payload, "No payload");
             }
             printf("Received RESPONSE from %s: %s\n",
                    inet_ntoa(*(struct in_addr *)&ip->saddr), resp_payload);
         }
     }
 
     close(sockfd);
     return 0;
 }
 