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

int main(int argc,char* argv[])
{
 
    if(argc<3)
    {
        printf("Missing Port/IP Number. Usage: %s <Server IP> <Server Port>\n", argv[0]);
        exit(EXIT_FAILURE); 
    }
    int port = atoi(argv[2]);

    int client_fd;
    struct sockaddr_in server_addr;

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) 
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    printf("Connected to My_SMTP server\n");

    char buff[4096],rec[4096];
    while (1)
    {
        printf("> ");
        if(fgets(buff,sizeof(buff),stdin)==NULL)
        {
            break;
        }
        while (strncmp(buff,"\n",1)==0)
        {
            if(fgets(buff,sizeof(buff),stdin)==NULL)
            {
                continue;
            }
        }
        

        buff[strcspn(buff, "\r\n")] = 0;
        int s = send(client_fd, buff, strlen(buff), 0);
        if (s <= 0) 
        {
            perror("Connection closed by server");
            break;
        }

        if(strcmp(buff,"DATA")==0)
        {
            int bytes = recv(client_fd, rec, sizeof(rec) - 1, 0);
            if (bytes <= 0) 
            {
                perror("Connection closed by server");
                break;
            }
            rec[bytes] = '\0';
            printf("%s",rec);
            if(strcmp(rec,"500 SERVER ERROR\n")==0) continue;

            while(1)
            {
                memset(buff,0,sizeof(buff));
                if (fgets(buff, sizeof(buff), stdin) == NULL) break;
                if (strcmp(buff, ".\n") == 0) 
                {
                    send(client_fd,"\n.\n",3,0);
                    break;
                }
                send(client_fd, buff, strlen(buff), 0);
            }
        }

        memset(rec,0,sizeof(rec));
        int bytes = recv(client_fd, rec, sizeof(rec) - 1, 0);
        if (bytes <= 0) 
        {
            perror("Connection closed by server2");
            break;
        }
        if(strncmp(rec + bytes - 3, "\n.\n", 3) != 0)
            printf("%s",rec);
        else 
        {
            printf("%.*s", bytes - 3, rec);
            continue;
        }
        if(strcmp(rec,"200 Goodbye\n")==0)
        {
            break;
        }

        if(strncmp(buff,"LIST",4)==0 || strncmp(buff,"GET_MAIL",8)==0)
        {
            while(1)
            {
                memset(rec,0,sizeof(rec));
                int bytes = recv(client_fd, rec, sizeof(rec) - 1, 0);
                if (bytes < 0) 
                {
                    perror("Connection closed by server1");
                    exit(EXIT_FAILURE);
                }
                rec[bytes] = '\0';
                
                if (bytes >= 3 && strncmp(rec + bytes - 3, "\n.\n", 3) == 0) 
                {
                    printf("%.*s", bytes - 3, rec);
                    break;
                }
                printf("%s",rec);
            }
            continue;
        }
    }
    return 0;
}