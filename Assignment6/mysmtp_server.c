//==============================
// Assignment 6 Submission
// Name: Golla Meghanandh Manvith Prabhash
// Roll number: 22CS30027
//==============================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/signal.h>
#include <semaphore.h>
#include <time.h>
#include <errno.h>

int server_fd;
#define INIT_BUF_SIZE 4096  // Initial buffer size for dynamic allocation
struct sembuf pop, vop;     // Semaphore operation structures

// Macros for semaphore operations
// P operation (wait/decrement)
#define P(s) do { if (semop(s, &pop, 1) == -1) { perror("P operation failed"); exit(EXIT_FAILURE); } } while(0)
// V operation (signal/increment)
#define V(s) do { if (semop(s, &vop, 1) == -1) { perror("V operation failed"); exit(EXIT_FAILURE); } } while(0)

// Global semaphore identifiers
int Sem,PrintSem;

/**
 * Converts email address to lowercase for case-insensitive comparison
 * @param email Pointer to email string
 */
void normalize_email(char *email) 
{
    for (int i = 0; email[i]; i++) if (email[i] >= 'A' && email[i] <= 'Z')  email[i] = email[i] + ('a' - 'A');
}

/**
 * Signal handler for SIGCHLD to prevent zombie processes
 * @param signo Signal number
 */
void sigchld_handler(int signo) 
{
    (void)signo;  // Avoid unused parameter warning
    while (waitpid(-1, NULL, WNOHANG) > 0);  // Reap all terminated child processes
}

/**
 * Signal handler for SIGINT (Ctrl+C)
 * Cleans up semaphores before exiting
 * @param sig Signal number
 */
void sigHandler(int sig) 
{
    semctl(Sem, 0, IPC_RMID);       // Remove mailbox semaphore
    semctl(PrintSem,0,IPC_RMID);    // Remove print semaphore
    close(server_fd);
    printf("\n~~~~~~~ Successfully Removed the Shared Memory ~~~~~~~\n");
    exit(EXIT_SUCCESS);
}

int main(int argc,char* argv[])
{
    // Set up signal handlers
    signal(SIGCHLD, sigchld_handler);  // Handle child process termination
    signal(SIGINT,sigHandler);         // Handle Ctrl+C gracefully

    // Initialize semaphore operation structures
    pop.sem_num = 0;
    vop.sem_num = 0;
    pop.sem_flg = 0;
    vop.sem_flg = 0;
    pop.sem_op = -1;  // Decrement operation
    vop.sem_op = 1;   // Increment operation

    // Create semaphores for synchronization
    Sem = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT | IPC_EXCL);      // Mailbox access semaphore
    PrintSem = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT | IPC_EXCL); // Print operation semaphore
    
    if(Sem == -1)
    {
        printf("Sem Creation Error\n");
        exit(EXIT_FAILURE);
    }

    // Initialize semaphores to 1 (unlocked)
    semctl(Sem, 0, SETVAL, 1);
    semctl(PrintSem, 0, SETVAL, 1);

    // Validate command-line arguments
    if(argc<2)
    {
        printf("Missing Port Number. Usage: %s <Port_Number>\n", argv[0]);
        exit(EXIT_FAILURE); 
    }
    int port = atoi(argv[1]);  // Convert port number from string to integer

    // Socket variables
    int client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Create server socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) 
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set up server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);            // Convert to network byte order
    server_addr.sin_addr.s_addr = INADDR_ANY;      // Listen on all interfaces

    // Bind socket to address and port
    if(bind (server_fd,(struct sockaddr*)&server_addr,sizeof(server_addr))<0)
    {
        perror("Binding Failed ");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) < 0)  // Queue up to 5 connection requests
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    
    struct stat st = {0};
    
    // Create mailbox directory if it doesn't exist
    if (stat("mailbox", &st) == -1) 
    {
        if (mkdir("mailbox", 0700) == -1) 
        {
            perror("Failed to create mailbox directory");
            exit(EXIT_FAILURE);
        }
    }

    int fd = - 1;  // Placeholder for file descriptor
    printf("Server listening on port %d...\n", port);
    
    // Main server loop
    while (1) 
    {
        // Accept client connection
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) 
        {
            P(PrintSem);  // Lock print semaphore
            perror("Accept failed");
            V(PrintSem);  // Release print semaphore
            continue;
        }

        // Fork a child process to handle the client
        if (fork() == 0) 
        {
            signal(SIGINT,SIG_DFL);  // Reset SIGINT handler for child process
            P(PrintSem);  // Lock print semaphore
            printf("Client connected : %s\n",inet_ntoa(client_addr.sin_addr)); 
            V(PrintSem);  // Release print semaphore
            
            // Variables for client session
            int reclen;
            char recbuf[4096];
            char recipient_email[64];
            char sender_email[64];

            // State flags for session
            int rechel = 0;     // HELO command received
            int recfrom = 0;    // MAIL FROM command received
            int recto = 0;      // RCPT TO command received
            char client_id[64] = {0};  // Client identifier
            
            // Client command processing loop
            while ((reclen=recv(client_fd,recbuf,sizeof(recbuf)-1,0))>0)
            {
                recbuf[reclen]='\0';  // Null-terminate received data
                
                // Check if HELO command was received first
                if(!rechel && (strncmp(recbuf,"HELO",4)))
                {
                    send(client_fd,"403 FORBIDDEN\n",14,0);
                    send(client_fd, "\n.\n", 3, 0);
                    continue;
                }   
                
                // Process HELO command
                if(!rechel && !(strncmp(recbuf,"HELO",4)))
                {
                    normalize_email(recbuf + 4);  // Make email case-insensitive
                    char *Helo = strtok(recbuf, " ");
                    char *temp_id = strtok(NULL, " \r\n");  // Get client ID
                    if (temp_id == NULL) 
                    {
                        send(client_fd, "400 ERR\n", 8, 0);
                        continue;
                    }
                    strncpy(client_id, temp_id, sizeof(client_id) - 1);  // Store client ID
                    client_id[sizeof(client_id) - 1] = '\0';  // Ensure null-termination
                    
                    P(PrintSem);  // Lock print semaphore
                    printf("HELO received from %s\n",client_id);
                    V(PrintSem);  // Release print semaphore

                    rechel = 1;  // Mark HELO as received
                    send(client_fd,"200 OK\n",7,0);
                    continue;
                }
                
                // Process QUIT command
                if(!(strncmp(recbuf,"QUIT",4)))
                {
                    rechel = 1;
                    send(client_fd,"200 Goodbye\n",12,0);

                    P(PrintSem);  // Lock print semaphore
                    printf("client disconnected\n");
                    V(PrintSem);  // Release print semaphore

                    break;  // Exit client processing loop
                }
                
                // Process MAIL FROM command
                if(!(strncmp(recbuf,"MAIL FROM:",10)))
                {
                    normalize_email(recbuf + 10);  // Make email case-insensitive
                    char *mail = strtok(recbuf, " ");
                    char *from = strtok(NULL, " ");
                    char *email = strtok(NULL, " ");
                    
                    // Validate email format
                    if (email == NULL )
                    {
                        send(client_fd,"400 ERR\n",8,0);
                        continue;
                    }
                    
                    if (strlen(email) < 5 || !strchr(email, '@')) 
                    {
                        send(client_fd,"401 NOT FOUND\n",14,0);
                        continue;
                    }
                    
                    size_t email_len = strlen(email);
                    size_t client_id_len = strlen(client_id);
                    
                    // Verify email domain matches client ID
                    if (email_len < client_id_len || strcmp(email + email_len - client_id_len, client_id) != 0) 
                    {
                        send(client_fd, "403 FORBIDDEN - ID MISMATCH\n", 29, 0);
                        continue;
                    }
                    
                    recfrom = 1;  // Mark MAIL FROM as received

                    P(PrintSem);  // Lock print semaphore
                    printf("MAIL FROM: %s\n",email);
                    V(PrintSem);  // Release print semaphore
                    
                    strcpy(sender_email,email);  // Store sender email
                    send(client_fd,"200 OK\n",7,0);
                    continue;
                }
                
                // Process RCPT TO command
                if(!(strncmp(recbuf,"RCPT TO:",8)))
                {
                    normalize_email(recbuf + 8);  // Make email case-insensitive
                    char *rcpt = strtok(recbuf, " ");
                    char *to = strtok(NULL, " ");
                    char *email = strtok(NULL, " ");
                    
                    // Validate email format
                    if (email == NULL )
                    {
                        send(client_fd,"400 ERR\n",8,0);
                        continue;
                    }
                    
                    if (strlen(email) < 5 || !strchr(email, '@')) 
                    {
                        send(client_fd,"401 NOT FOUND\n",14,0);
                        continue;
                    }

                    size_t email_len = strlen(email);
                    size_t client_id_len = strlen(client_id);

                    // Verify email domain matches client ID
                    if (email_len < client_id_len || strcmp(email + email_len - client_id_len, client_id) != 0) 
                    {
                        send(client_fd, "403 FORBIDDEN - ID MISMATCH\n", 29, 0);
                        continue;
                    }
                    
                    recto = 1;  // Mark RCPT TO as received

                    P(PrintSem);  // Lock print semaphore
                    printf("RCPT TO : %s\n",email);
                    V(PrintSem);  // Release print semaphore

                    strcpy(recipient_email,email);  // Store recipient email
                    send(client_fd,"200 OK\n",7,0);    
                    continue;
                }
                
                // Process DATA command
                if(!(strncmp(recbuf,"DATA",4)))
                {
                    // Ensure proper command sequence
                    if(!recfrom || !recto) 
                    {
                        send(client_fd,"500 SERVER ERROR\n",17,0);
                        continue;
                    }

                    send(client_fd,"Enter Your Message (end with a single dot '.')\n",47,0); 

                    // Prepare to store the message
                    char filename[512];
                    char date_str[32];
                    time_t t;
                    struct tm *today;

                    // Get current date and time
                    time(&t);
                    today = localtime(&t);

                    FILE *fp;
                    snprintf(filename, sizeof(filename), "mailbox/%s.txt", recipient_email);
                    
                    // Allocate buffer for message with dynamic resizing
                    char *buffer = malloc(INIT_BUF_SIZE); 
                    int buffer_len = 0;
                    int buffer_size = INIT_BUF_SIZE;

                    // Read message data until termination sequence
                    while (1)
                    {
                        char recbuf[1024];
                        memset(recbuf, 0, sizeof(recbuf));
                        int bytes = recv(client_fd, recbuf, sizeof(recbuf) - 1, 0);

                        if (bytes < 0) 
                        {
                            // Handle non-blocking socket errors
                            if (errno == EAGAIN || errno == EWOULDBLOCK)
                            {
                                usleep(10000);  // Short delay to prevent busy waiting
                                continue;
                            }
                            perror("Connection closed or error...");
                            break;
                        }

                        if (bytes == 0) 
                        {
                            // Connection closed
                            break;
                        }

                        // Resize buffer if needed
                        if (buffer_len + bytes >= buffer_size)
                        {
                            buffer_size *= 2;  // Double buffer size
                            buffer = realloc(buffer, buffer_size);
                            if (!buffer)
                            {
                                perror("Memory allocation failed");
                                exit(EXIT_FAILURE);
                            }
                        }

                        // Append received data to buffer
                        memcpy(buffer + buffer_len, recbuf, bytes);
                        buffer_len += bytes;

                        // Check for termination sequence "\n.\n"
                        if (buffer_len >= 3 && 
                            strncmp(buffer + buffer_len - 3, "\n.\n", 3) == 0)
                        {
                            buffer_len -= 3;  // Remove termination sequence
                            break;
                        }
                    }

                    // Lock mailbox for writing
                    P(Sem);

                    fp = fopen(filename, "a");
                    if (fp == NULL) 
                    {
                        perror("Failed to open mailbox file");
                        free(buffer);
                        V(Sem);  // Release semaphore
                        continue;
                    }

                    // Write email with metadata
                    fprintf(fp, "~~~ START ~~~\n");
                    fprintf(fp, "From: %s\n", sender_email);
                    fprintf(fp, "Date: %02d-%02d-%04d\n", 
                            today->tm_mday, today->tm_mon + 1, today->tm_year + 1900);
                    fprintf(fp, "%.*s", buffer_len, buffer);
                    fprintf(fp, "~~~ END ~~~\n\n");

                    fclose(fp);
                    V(Sem);  // Release mailbox semaphore

                    free(buffer);  // Free dynamically allocated buffer

                    // Reset state for next message
                    recfrom = 0;
                    recto = 0;
                    send(client_fd,"200 OK\n",7,0); 
                    P(PrintSem);  // Lock print semaphore
                    printf("DATA received,message stored.\n");
                    V(PrintSem);  // Release print semaphore
                    continue;
                }
                
                // Process LIST command
                if(!(strncmp(recbuf,"LIST",4)))
                {
                    normalize_email(recbuf + 4);  // Make email case-insensitive
                    char *list = strtok(recbuf, " ");
                    char *email = strtok(NULL, " ");

                    // Validate email format
                    if (email == NULL )
                    {
                        send(client_fd,"400 ERR\n",8,0);
                        send(client_fd, "\n.\n", 3, 0);
                        continue;
                    }

                    if (strlen(email) < 5 || !strchr(email, '@')) 
                    {
                        send(client_fd,"401 NOT FOUND\n",14,0);
                        send(client_fd, "\n.\n", 3, 0);
                        continue;
                    }

                    size_t email_len = strlen(email);
                    size_t client_id_len = strlen(client_id);

                    // Verify email domain matches client ID
                    if (email_len < client_id_len || strcmp(email + email_len - client_id_len, client_id) != 0) 
                    {
                        send(client_fd, "403 FORBIDDEN - ID MISMATCH\n", 29, 0);
                        send(client_fd, "\n.\n", 3, 0);
                        continue;
                    }
                    
                    P(PrintSem);  // Lock print semaphore
                    printf("LIST %s\n",email);
                    V(PrintSem);  // Release print semaphore

                    char filename[512];
                    char buffer[4096];
                    FILE *fp;
                    int email_count = 0;
                    char sender[256];
                    char date[32];

                    snprintf(filename, sizeof(filename), "mailbox/%s.txt", email);
                    
                    // Lock mailbox for reading
                    P(Sem);
                    fp = fopen(filename, "r");
                    if (fp == NULL) 
                    {
                        send(client_fd, "401 NOT FOUND\n", 14, 0);
                        send(client_fd, "\n.\n", 3, 0);
                        continue;
                    }
                    
                    send(client_fd,"200 OK\n",7,0);    

                    // Parse mailbox file and send email list
                    while (fgets(buffer, sizeof(buffer), fp) != NULL) 
                    {
                        if (strncmp(buffer, "~~~ START ~~~", 13) == 0) 
                        {
                            email_count++;
                            
                            // Extract sender info
                            if (fgets(buffer, sizeof(buffer), fp) && strncmp(buffer, "From: ", 6) == 0) 
                            {
                                strncpy(sender, buffer + 6, sizeof(sender) - 1);
                                sender[strcspn(sender, "\n")] = 0;  // Remove newline
                            }
                            
                            // Extract date info
                            if (fgets(buffer, sizeof(buffer), fp) && strncmp(buffer, "Date: ", 6) == 0) 
                            {
                                strncpy(date, buffer + 6, sizeof(date) - 1);
                                date[strcspn(date, "\n")] = 0;  // Remove newline
                            }
                
                            // Format and send email entry
                            snprintf(buffer, sizeof(buffer), "%d: Email from %s (%s)\n", email_count, sender, date);
                            send(client_fd, buffer, strlen(buffer), 0);
                        }
                    }
                    fclose(fp);
                    V(Sem);  // Release mailbox semaphore

                    // Report if no emails found
                    if(!email_count) send(client_fd,"NO EMAILS FOUND\n",16,0);
                    send(client_fd, "\n.\n", 3, 0);  // Send termination sequence
                    P(PrintSem);  // Lock print semaphore
                    printf("Emails retrived; list sent.\n");
                    V(PrintSem);  // Release print semaphore
                    continue;
                }
                
                // Process GET_MAIL command
                if(!(strncmp(recbuf,"GET_MAIL",7)))
                {
                    normalize_email(recbuf + 7);  // Make email case-insensitive
                    char *get_task = strtok(recbuf, " ");
                    char *email = strtok(NULL, " ");
                    char *id_str = strtok(NULL, " ");

                    // Validate command parameters
                    if (email == NULL || id_str == NULL)
                    {
                        send(client_fd,"400 ERR\n",8,0);
                        send(client_fd, "\n.\n", 3, 0);
                        continue;
                    }

                    if(strlen(email) < 5 || !strchr(email, '@')) 
                    {
                        send(client_fd, "401 NOT FOUND\n", 14, 0);
                        send(client_fd, "\n.\n", 3, 0);
                        continue;
                    }

                    size_t email_len = strlen(email);
                    size_t client_id_len = strlen(client_id);

                    // Verify email domain matches client ID
                    if (email_len < client_id_len || strcmp(email + email_len - client_id_len, client_id) != 0) 
                    {
                        send(client_fd, "403 FORBIDDEN - ID MISMATCH\n", 29, 0);
                        send(client_fd, "\n.\n", 3, 0);
                        continue;
                    }
                    P(PrintSem);  // Lock print semaphore
                    printf("GET_MAIL %s %s\n",email,id_str);
                    V(PrintSem);  // Release print semaphore
                    int id = atoi(id_str);  // Convert email ID to integer

                    char filename[512];
                    char buffer[4096];
                    FILE *fp;
                    int email_count = 0;
                    int found = 0;
                    
                    snprintf(filename, sizeof(filename), "mailbox/%s.txt", email);
                    
                    // Lock mailbox for reading
                    P(Sem);
                    fp = fopen(filename, "r");
                    if (fp == NULL) 
                    {
                        send(client_fd, "401 NOT FOUND\n", 14, 0);
                        send(client_fd, "\n.\n", 3, 0);
                        continue;
                    }
                    
                    // Search for the specified email by ID
                    while (fgets(buffer, sizeof(buffer), fp) != NULL)
                    {
                        if (strncmp(buffer, "~~~ START ~~~", 13) == 0) 
                        {
                            email_count++;
                            if (email_count == id) 
                            {
                                send(client_fd,"200 OK\n",7,0);
                                found = 1;
                                // Send the entire email content
                                while (fgets(buffer, sizeof(buffer), fp) != NULL) 
                                {
                                    if (strncmp(buffer, "~~~ END ~~~", 11) == 0) break;
                                    send(client_fd, buffer, strlen(buffer), 0);
                                }
                                break;
                            }
                        }
                    }
                    if (!found) send(client_fd, "401 NOT FOUND\n", 14, 0);
                    else 
                    {
                        P(PrintSem);  // Lock print semaphore
                        printf("Email with id %d sent.\n", id);
                        V(PrintSem);  // Release print semaphore
                    }
                    send(client_fd, "\n.\n", 3, 0);  // Send termination sequence
                    fclose(fp);
                    V(Sem);  // Release mailbox semaphore
                    continue;
                }
                
                // Send error for unrecognized commands
                send(client_fd,"400 ERR\n",8,0);        
            }
            close(client_fd);  // Close client connection in child process
            exit(EXIT_SUCCESS);  // Exit child process
        }
        close(client_fd);  // Close client connection in parent process
    }
    return 0;
}