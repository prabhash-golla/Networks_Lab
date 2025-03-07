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
	int			sockfd ;
	struct sockaddr_in	serv_addr;
	int i;
	char buf[BUFFER_SIZE];
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{
		perror("Unable to create socket\n");
		exit(0);
	}
	serv_addr.sin_family	= AF_INET;
    // serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	inet_aton("127.0.0.1", &serv_addr.sin_addr);
	serv_addr.sin_port	= htons(20000);
	if ((connect(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr))) < 0) 
	{
		perror("Unable to connect to server\n");
		exit(0);
	}
    for(i=0; i < 100; i++) buf[i] = '\0';
	recv(sockfd, buf, 100, 0);
	printf("%s\n", buf);
	strcpy(buf,"Message from client");
	send(sockfd, buf, strlen(buf) + 1, 0);
	char filename[BUFFER_SIZE];
	FILE* fptr = NULL;
    char c;
    do
    {
        c = 'k';
        printf("Enter The File Name : ");
        scanf("%s",filename);
        fptr = fopen(filename,"r");
        while (fptr==NULL)
        {
            printf("NOTFOUND %s\n",filename);
            printf("Enter The File Name : ");
            scanf("%s",filename);
            fptr = fopen(filename,"r");
        }
        char KEY[BUFFER_SIZE];
        for(i=0; i < BUFFER_SIZE; i++) KEY[i] = '\0';
        printf("Enter the KEY (26 characters) : ");
        scanf("%s",KEY);
        while (strlen(KEY)<26)
        {
            printf("Error,Entered KEY length is less than 26,Entered KEY was : %s \n",KEY);
            printf("Enter the KEY (26 characters) : ");
            scanf("%s",KEY);
        }
        send(sockfd, KEY, strlen(KEY), 0);
        printf("Entered KEY is : %s\n",KEY);
        recv(sockfd, buf, BUFFER_SIZE, 0);
        for(i=0; i < BUFFER_SIZE; i++) buf[i] = '\0';
        while (fgets(buf, BUFFER_SIZE, fptr))
        {
            send(sockfd, buf, strlen(buf), 0);
            for(i=0; i < BUFFER_SIZE; i++) buf[i] = '\0';
        }
        send(sockfd, "*", 1, 0);

        fclose(fptr);

        char enc_filename[104];
        sprintf(enc_filename, "%s.enc", filename);

        recv(sockfd, buf, BUFFER_SIZE, 0);
        send(sockfd, buf, strlen(buf), 0);

        FILE *encf = fopen(enc_filename, "w");
        if (!encf) 
        {
            perror("File open error");
            exit(EXIT_FAILURE);
        }
        while (1) 
        {
            int bytes_received = recv(sockfd, buf, BUFFER_SIZE, 0);
            if (bytes_received < 0) 
            {
                perror("recv error");
                break;
            }
            
            if (bytes_received == 0) 
            {
                printf("%s\n",buf);
                printf("End of file reached. Transfer complete.\n");
                break;
            }

            if (bytes_received >= 1 && strncmp(buf + bytes_received - 1, "*", 1) == 0) 
            {
                fwrite(buf, 1, bytes_received - 1, fptr);
                printf("Encrypted File Recived Successfully.\n");
                printf("Orignial File Name : %s , Encrypted File name %s\n",filename,enc_filename);
                break;
            }

            fwrite(buf, 1, bytes_received, fptr);
            memset(buf, 0, BUFFER_SIZE);
        }
        fclose(encf);        
        printf("Do you want to encrypt any other file (y/n) : ");
        while (c!='y'& c!='n' & c!='Y' & c!='N') 
        {   
            scanf(" %c",&c);
        }
        if(c=='y')
        {
            strcpy(buf,"Continue");
        }
        else
        {
            strcpy(buf,"Break");
        }
        send(sockfd, buf, strlen(buf) + 1, 0);
    } while (c=='y'|| c=='Y');
	return 0;
}
