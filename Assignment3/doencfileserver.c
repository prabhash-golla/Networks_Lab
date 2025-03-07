/*
=====================================
Assignment 3 Submission
Name: Golla Meghanandh Manvith Prabahash
Roll number: 22CS30027
Link of the pcap file: https://drive.google.com/file/d/1IqBw96w9q_kDb0jlj1NyT4thcaKzvnFT/view?usp=sharing
=====================================
*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>
#define BUFFER_SIZE 100

int main()
{
	int			sockfd, newsockfd ; /* Socket descriptors */
	int			clilen;
	struct sockaddr_in	cli_addr, serv_addr;
	int i;
	char buf[BUFFER_SIZE];
	char KEY[BUFFER_SIZE];
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{

		perror("Cannot create socket\n");
		exit(EXIT_FAILURE);
	}
	serv_addr.sin_family		= AF_INET;
	serv_addr.sin_addr.s_addr	= INADDR_ANY;
	serv_addr.sin_port		= htons(20000);
	if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
	{
		perror("Unable to bind local address ");
		exit(EXIT_FAILURE);
	}
	listen(sockfd, 5); 

	while (1) 
	{
		clilen = sizeof(cli_addr);
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr,&clilen) ;
		if (newsockfd < 0) 
		{
			perror("Accept error\n");
			exit(EXIT_FAILURE);
		}
        strcpy(buf,"Message from server");
		send(newsockfd, buf, strlen(buf) + 1, 0);
		recv(newsockfd, buf, 100, 0);
		printf("%s\n", buf);
        if (fork() == 0) 
		{
            char filename[250],encrypted_file[300];
            char temp1[BUFFER_SIZE],temp2[BUFFER_SIZE];
            int port;
            inet_ntop(cli_addr.sin_family, &(cli_addr.sin_addr), temp1, sizeof(temp1));
            printf("Client %s",temp1);
            port = htons(cli_addr.sin_port);
            printf(" at port %d connected\n",port);
            sprintf(temp2,"%d",port);
            sprintf(filename,"%s.%s.txt",temp1,temp2);
            snprintf(encrypted_file, sizeof(encrypted_file), "%s.enc", filename);
            do
            {
                printf("Strating for Port : %d....\n",port);
                for(i=0; i < BUFFER_SIZE; i++) KEY[i] = '\0';
                recv(newsockfd, KEY, sizeof(KEY), 0);
                printf("Recived KEY : %s\n", KEY);
                send(newsockfd, buf, strlen(buf) + 1, 0);
                FILE *fptr = fopen(filename, "w");
                while (1) 
                {
                    int bytes_received = recv(newsockfd, buf, BUFFER_SIZE, 0);
                    if (bytes_received < 0) {
                        perror("recv error");
                        break;
                    }
                    if (bytes_received == 0) break;
                    if (bytes_received >= 1 && strncmp(buf + bytes_received - 1, "*", 1) == 0) 
                    {
                        fwrite(buf, 1, bytes_received - 1, fptr); 
                        for(i=0; i < BUFFER_SIZE; i++) buf[i] = '\0';
                        break;
                    }
                    fwrite(buf, 1, bytes_received, fptr);  
                    for(i=0; i < BUFFER_SIZE; i++) buf[i] = '\0';
                }

                fclose(fptr);
                printf("File received successfully\n");
                
                FILE *une = fopen(filename, "r");
                FILE *en = fopen(encrypted_file, "w");
                if (!une || !en)
                {
                    perror("File error");
                    exit(EXIT_FAILURE);
                }

                char buffer[BUFFER_SIZE];
                while (fgets(buffer, BUFFER_SIZE, une))
                {
                    for (int i = 0; buffer[i] != '\0'; i++) 
                    {
                        if (buffer[i] >= 'A' && buffer[i] <= 'Z') 
                        {
                            buffer[i] = KEY[buffer[i] - 'A'];
                        } 
                        else if (buffer[i] >= 'a' && buffer[i] <= 'z') 
                        {
                            buffer[i] = KEY[buffer[i] - 'a'] + 32;
                        }
                    }
                    fputs(buffer, en);
                }

                fclose(une);
                fclose(en);

                strcpy(buf,"File is Encrypted");
                send(newsockfd, buf, strlen(buf) + 1, 0);
                recv(newsockfd, buf, BUFFER_SIZE, 0);

                FILE *enc_file = fopen(encrypted_file, "r");
                while (fgets(buf, BUFFER_SIZE, enc_file))
                {
                    send(newsockfd, buf, strlen(buf), 0);
                    for(i=0; i < BUFFER_SIZE; i++) buf[i] = '\0';
                }
                send(newsockfd, "*", 1, 0);
                fclose(enc_file);

                printf("Encrypted file sent to the client: %s\n", encrypted_file);
                recv(newsockfd, buf, BUFFER_SIZE, 0);
            } while (!strcmp(buf,"Continue"));
            
            close(newsockfd);
        }
        else 
        {
            close(newsockfd);
        }
	}
	return 0;
}
