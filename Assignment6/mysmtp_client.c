//==============================
// Assignment 6 Submission
// Name: Golla Meghanandh Manvith Prabhash
// Roll number: 22CS30027
//==============================
#include <stdio.h>      // Standard I/O functions
#include <stdlib.h>     // Standard library functions like exit()
#include <string.h>     // String manipulation functions
#include <unistd.h>     // UNIX standard functions like close()
#include <arpa/inet.h>  // Internet operations
#include <sys/types.h>  // Various data types
#include <sys/socket.h> // Socket definitions
#include <sys/wait.h>   // Wait for process termination

int main(int argc, char* argv[])
{
    // Check if correct number of arguments are provided
    if(argc < 3)
    {
        printf("Missing Port/IP Number. Usage: %s <Server IP> <Server Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int port = atoi(argv[2]);  // Convert port number string to integer
    int client_fd;             // Socket file descriptor
    struct sockaddr_in server_addr;  // Server address structure
    
    // Create TCP socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    server_addr.sin_family = AF_INET;  // IPv4
    server_addr.sin_port = htons(port);  // Convert to network byte order
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);  // Set IP address
    
    // Connect to server
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        close(client_fd);  // Close socket before exiting
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to My_SMTP server\n");
    
    char buff[4096], rec[4096];  // Buffers for sending and receiving data
    
    // Main communication loop
    while (1)
    {
        printf("> ");  // Print prompt
        
        // Read user input
        if(fgets(buff, sizeof(buff), stdin) == NULL)
        {
            break;  // Exit on EOF
        }
        
        // Skip empty lines
        while (strncmp(buff, "\n", 1) == 0)
        {
            printf("> ");  // Print prompt
            if(fgets(buff, sizeof(buff), stdin) == NULL) continue;
        }
        
        // Remove newline characters from the buffer
        buff[strcspn(buff, "\r\n")] = 0;
        
        // Send command to server
        int s = send(client_fd, buff, strlen(buff), 0);
        if (s <= 0)
        {
            perror("Connection closed by server");
            break;
        }
        
        // Special handling for DATA command
        if(strcmp(buff, "DATA") == 0)
        {
            // Receive server response for DATA command
            int bytes = recv(client_fd, rec, sizeof(rec) - 1, 0);
            if (bytes <= 0)
            {
                perror("Connection closed by server");
                break;
            }
            rec[bytes] = '\0';
            // Handle special response format ending with '\n.\n'
            if(strncmp(rec + bytes - 3, "\n.\n", 3) != 0) printf("%s", rec);
            else printf("%.*s", bytes - 3, rec);            
            // If server reports error, continue to next iteration
            if(strncmp(rec, "500 SERVER ERROR\n",17) == 0) continue;
            if(strncmp(rec, "403 FORBIDDEN\n",14) == 0) continue;
            
            // Read mail content until '.' on a line by itself
            while(1)
            {
                memset(buff, 0, sizeof(buff));
                if (fgets(buff, sizeof(buff), stdin) == NULL) break;
                
                // End of mail content
                if (strcmp(buff, ".\n") == 0)
                {
                    send(client_fd, "\n.\n", 3, 0);
                    break;
                }
                
                // Send mail content line to server
                send(client_fd, buff, strlen(buff), 0);
            }
        }
        
        // Clear receive buffer
        memset(rec, 0, sizeof(rec));
        
        // Receive server response
        int bytes = recv(client_fd, rec, sizeof(rec) - 1, 0);
        if (bytes <= 0)
        {
            perror("Connection closed by server2");
            break;
        }
        
        // Handle special response format ending with '\n.\n'
        if(strncmp(rec + bytes - 3, "\n.\n", 3) != 0) printf("%s", rec);
        else
        {
            printf("%.*s", bytes - 3, rec);
            continue;
        }
        
        // Break if server says goodbye
        if(strcmp(rec, "200 Goodbye\n") == 0) break;
        
        // Special handling for LIST and GET_MAIL commands
        if(strncmp(buff, "LIST", 4) == 0 || strncmp(buff, "GET_MAIL", 8) == 0)
        {
            // Continue receiving data until end marker '\n.\n'
            while(1)
            {
                memset(rec, 0, sizeof(rec));
                int bytes = recv(client_fd, rec, sizeof(rec) - 1, 0);
                if (bytes < 0)
                {
                    perror("Connection closed by server1");
                    exit(EXIT_FAILURE);
                }
                rec[bytes] = '\0';
                
                // Check for end marker
                if (bytes >= 3 && strncmp(rec + bytes - 3, "\n.\n", 3) == 0)
                {
                    printf("%.*s", bytes - 3, rec);  // Print without end marker
                    break;
                }
                
                printf("%s", rec);
            }
            continue;
        }
    }
    
    return 0;  // Exit successfully
}