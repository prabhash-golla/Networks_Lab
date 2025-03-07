/*
=====================================
Assignment 4 Submission
Name: Golla Meghanandh Manvith Prabahash
Roll number: 22CS30027
=====================================
*/
#include <stdio.h>
#include "ksocket.h"
#include <fcntl.h>

// ------- ASSUMING EOF CHARACTER AS ~ ------- //
int main() 
{
    int sockfd,Port1,Port2;
    struct sockaddr_in Source, Destination;
    char src_addr[INET_ADDRSTRLEN], dest_addr[INET_ADDRSTRLEN];
    char buf[512];
    socklen_t addr_size = sizeof(Source);

    // ------- SOURCE BLOCK ------- //
    Source.sin_family = AF_INET;
    printf("=>Enter Your Address : ");
    scanf("%s",src_addr);
    if (inet_aton(src_addr, &Source.sin_addr) == 0) 
    {
        printf("-------Invalid Your IP address-------\n");
        return 1;
    }
    printf("=>Enter Your Port Number : ");
    scanf("%d",&Port1);
    Source.sin_port = htons(Port1);

    // ------- DESTINATION BLOCK ------- //
    Destination.sin_family = AF_INET;
    printf("=>Enter Senders Address : ");
    scanf("%s", dest_addr);  // Read as a string
    if (inet_aton(dest_addr, &Destination.sin_addr) == 0) 
    {
        printf("-------Invalid Senders IP address-------\n");
        return 1;
    }
    printf("=>Enter Senders Port Number : ");
    scanf("%d",&Port2);
    Destination.sin_port = htons(Port2);

    // ------- SOCKET CREATION BLOCK ------- //
    sockfd = k_socket(AF_INET, SOCK_KTP, 0);
    if (sockfd < 0) 
    {
        perror("-------Error in socket creation-------");
        exit(EXIT_FAILURE);
    }
    printf("------- Socket Creation Successfull -------\n");

    // ------- BIND CREATION BLOCK ------- //
    int l = k_bind(sockfd, (struct sockaddr *)&Source, (struct sockaddr *)&Destination);
    if (l < 0) 
    {
        perror("-------Error in binding-------");
        exit(EXIT_FAILURE);
    }
    printf("------- Bind Creation Successfull -------\n");

    // ------- FILE NAME INPUT BLOCK ------- //
    char a[50];
    printf("=>Enter File Name To Store Recived Packets : ");
    scanf("%s",a);

    // ------- FILE OPENING BLOCK ------- //
    int fd=open(a, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if(fd<0)
    {
        perror("------- Error in opening/creating file -------\n");
        return 1;
    }

    int recvlen;

    // ------- RECIVING BLOCK ------- //
    while(1)
    {
        while((recvlen = k_recvfrom(sockfd, buf, 512, 0, (struct sockaddr*)&Source,&addr_size ))<=0)
        if (recvlen <= 0) 
        {
            if(errno==ENOMESSAGE) continue;
            else perror("-------Error in sending-------\n");
        }
        if(buf[0]=='~' && recvlen==1) break;
        printf("------- Recived & Writing %d bytes -------\n", recvlen);
        if(write(fd, buf, recvlen)<0)
        {
            perror("-------Error in writing to file-------\n");
            return 1;
        }
    }
    printf("------- Recived EOF as '~' -------\n");
    close(fd); 
    // sleep(30);  //UNCOMMENT IF NEEDED

    // ------- CLOSING BLOCK ------- //
    if (k_close(sockfd) < 0) 
    {
        perror("-------Error in closing-------");
        exit(EXIT_FAILURE);
    }

    return 0;
}
