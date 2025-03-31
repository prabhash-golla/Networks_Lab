/*
 * cldp_server.c
 *
 * A simple CLDP server that:
 *  - Broadcasts a HELLO message every 10 seconds.
 *  - Listens for QUERY messages and responds with a RESPONSE
 *    containing the hostname and current timestamp.
 *
 * Compile with:
 *   gcc -o cldp_server cldp_server.c -lpthread
 *
 * Run with sudo:
 *   sudo ./cldp_server [local_ip]
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
 #include <pthread.h>
 #include <time.h>
 #include <sys/time.h>
 #include <netdb.h>
 
 #define PROTO_CLDP   253
 #define MSG_HELLO    0x01
 #define MSG_QUERY    0x02
 #define MSG_RESPONSE 0x03
 
 // Custom CLDP header (8 bytes)
 struct cldp_header {
     uint8_t  msg_type;      // Message type: HELLO, QUERY, RESPONSE
     uint8_t  payload_len;   // Payload length (in bytes)
     uint16_t transaction_id;// Transaction ID (for matching queries/responses)
     uint32_t reserved;      // Reserved (set to 0)
 } __attribute__((packed));
 
 // Compute the standard IP header checksum
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
 
 // Global socket and local source IP (network byte order)
 int sockfd;
 uint32_t source_ip;
 
 // Helper function: Build and send an IP packet carrying a CLDP message.
 void send_packet(uint32_t dest_ip, uint8_t msg_type, uint16_t transaction_id,
                  const char *payload, int payload_len)
 {
     char buffer[1500];
     memset(buffer, 0, sizeof(buffer));
 
     // Pointers to the IP header and our custom CLDP header within the buffer
     struct iphdr *ip = (struct iphdr *)buffer;
     struct cldp_header *cldp = (struct cldp_header *)(buffer + sizeof(struct iphdr));
 
     int total_payload = sizeof(struct cldp_header) + payload_len;
     int packet_len = sizeof(struct iphdr) + total_payload;
 
     /* Fill in the IP header */
     ip->ihl      = 5;          // 5 x 32-bit words = 20 bytes
     ip->version  = 4;
     ip->tos      = 0;
     ip->tot_len  = htons(packet_len);
     ip->id       = htons(rand() % 65535);
     ip->frag_off = 0;
     ip->ttl      = 64;
     ip->protocol = PROTO_CLDP;  // Our custom protocol number
     ip->saddr    = source_ip;   // Local source IP (in network byte order)
     ip->daddr    = dest_ip;     // Destination IP
     ip->check    = 0;
     ip->check    = compute_checksum((unsigned short *)ip, sizeof(struct iphdr));
 
     /* Fill in the custom CLDP header */
     cldp->msg_type       = msg_type;
     cldp->payload_len    = payload_len;
     cldp->transaction_id = htons(transaction_id);
     cldp->reserved       = 0;
 
     // Copy the payload (if any) right after the custom header.
     if (payload_len > 0 && payload != NULL)
         memcpy(buffer + sizeof(struct iphdr) + sizeof(struct cldp_header),
                payload, payload_len);
 
     // Destination address structure for sendto()
     struct sockaddr_in dest;
     dest.sin_family      = AF_INET;
     dest.sin_addr.s_addr = dest_ip;
 
     if (sendto(sockfd, buffer, packet_len, 0, (struct sockaddr *)&dest,
                sizeof(dest)) < 0) {
         perror("sendto failed");
     }
 }
 
 // Thread: Periodically send HELLO messages every 10 seconds.
 void *hello_sender(void *arg) {
     while (1) {
         uint16_t transaction_id = rand() % 65535;
         printf("Sending HELLO message\n");
         // For broadcast, use the broadcast IP address.
         uint32_t broadcast_ip = inet_addr("255.255.255.255");
         send_packet(broadcast_ip, MSG_HELLO, transaction_id, NULL, 0);
         sleep(10);
     }
     return NULL;
 }
 
 int main(int argc, char *argv[]) {
     srand(time(NULL));
 
     // Optionally, use the provided local IP; otherwise default to 127.0.0.1.
     if (argc > 1) {
         source_ip = inet_addr(argv[1]);
     } else {
         source_ip = inet_addr("127.0.0.1");
     }
 
     // Create a raw socket for our custom protocol.
     if ((sockfd = socket(AF_INET, SOCK_RAW, PROTO_CLDP)) < 0) {
         perror("Socket creation failed");
         exit(EXIT_FAILURE);
     }
 
     // Inform the kernel that IP headers are included in our packets.
     int one = 1;
     if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
         perror("Failed to set IP_HDRINCL");
         exit(EXIT_FAILURE);
     }
 
     // Allow broadcast messages.
     if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one)) < 0) {
         perror("Failed to set SO_BROADCAST");
         exit(EXIT_FAILURE);
     }
 
     // Start a thread to send HELLO messages periodically.
     pthread_t hello_thread;
     if (pthread_create(&hello_thread, NULL, hello_sender, NULL) < 0) {
         perror("Could not create HELLO sender thread");
         exit(EXIT_FAILURE);
     }
 
     // Main loop: Listen for incoming CLDP packets.
     while (1) {
         char buffer[1500];
         memset(buffer, 0, sizeof(buffer));
         struct sockaddr_in src_addr;
         socklen_t addr_len = sizeof(src_addr);
         int bytes_received = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                                       (struct sockaddr *)&src_addr, &addr_len);
         if (bytes_received < 0) {
             perror("recvfrom failed");
             continue;
         }
 
         // Parse the IP header.
         struct iphdr *ip = (struct iphdr *)buffer;
         // Filter out packets that aren’t using our custom protocol.
         if (ip->protocol != PROTO_CLDP)
             continue;
 
         // Parse the custom CLDP header.
         struct cldp_header *cldp = (struct cldp_header *)(buffer + sizeof(struct iphdr));
 
         // Process QUERY messages: respond with hostname and timestamp.
         if (cldp->msg_type == MSG_QUERY) {
             uint16_t transaction_id = ntohs(cldp->transaction_id);
             printf("Received QUERY from %s, transaction id: %u\n",
                    inet_ntoa(*(struct in_addr *)&ip->saddr), transaction_id);
 
             // Get the system's hostname.
             char hostname[256];
             if (gethostname(hostname, sizeof(hostname)) < 0) {
                 perror("gethostname failed");
                 strcpy(hostname, "unknown");
             }
 
             // Get the current time.
             struct timeval tv;
             gettimeofday(&tv, NULL);
             char timestamp[64];
             snprintf(timestamp, sizeof(timestamp), "%ld.%06ld", tv.tv_sec, tv.tv_usec);
 
             // Construct payload as "hostname|timestamp".
             char payload[512];
             int payload_len = snprintf(payload, sizeof(payload), "%s|%s", hostname, timestamp);
 
             // Send the RESPONSE back to the sender (use the source IP from the packet).
             uint32_t dest_ip = ip->saddr;
             send_packet(dest_ip, MSG_RESPONSE, transaction_id, payload, payload_len);
             printf("Sent RESPONSE to %s with payload: %s\n",
                    inet_ntoa(*(struct in_addr *)&ip->saddr), payload);
         }
     }
 
     close(sockfd);
     return 0;
 }
 