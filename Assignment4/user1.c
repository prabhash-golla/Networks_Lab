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

// ------- ASSUMING NO ~ CHARACTER AT EOF HAVING SIZE OF FORMATE 512*X+1 ------- //
int main() 
{
    int sockfd,Port1,Port2;
    struct sockaddr_in Source, Destination;
    char src_addr[INET_ADDRSTRLEN], dest_addr[INET_ADDRSTRLEN];
    char buf[513];

    // ------- SOURCE BLOCK ------- //
    Source.sin_family = AF_INET;
    printf("=>Enter Your Address : ");
    scanf("%s",src_addr);
    if (inet_aton(src_addr, &Source.sin_addr) == 0) 
    {
        printf("Invalid Your IP address.\n");
        return 1;
    }
    printf("=>Enter Your Port Number : ");
    scanf("%d",&Port1);
    Source.sin_port = htons(Port1);

    // ------- DESTINATION BLOCK ------- //
    Destination.sin_family = AF_INET;
    printf("=>Enter Recivers Address : ");
    scanf("%s", dest_addr);
    if (inet_aton(dest_addr, &Destination.sin_addr) == 0)
    {
        printf("Invalid Recivers IP address.\n");
        return 1;
    }
    printf("=>Enter Recivers Port Number : ");
    scanf("%d",&Port2);
    Destination.sin_port = htons(Port2);

    // ------- SOCKET CREATION BLOCK ------- //
    sockfd = k_socket(AF_INET, SOCK_KTP, 0);
    if (sockfd < 0) 
    {
        perror("------Error in socket creation--------");
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
    printf("=>Enter File Name To Send : ");
    scanf("%s",a);

    // ------- FILE OPEN BLOCK ------- //
    int fd=open(a, O_RDONLY);
    if(fd<0)
    {
        perror("------- Error in opening file -------\n");
        return 1;
    }

    int readlen,seq=1,Trasmission=0;
    for(int i=0;i<512;i++) buf[i]='\0';
    // ------- SENDING BLOCK ------- //
    while((readlen=read(fd, buf, 512))>0)
    {
        buf[512]='\0';
        int sendlen;
        while(1)
        {
            while((sendlen=k_sendto(sockfd, buf, readlen, 0, (struct sockaddr*)&Destination, sizeof(Destination)))<0 && errno==ENOSPACE) sleep(1);
            if(sendlen>=0) break;
            if(errno==ENOTBOUND) printf("-------Error : ENOTBOUND-------");
            else perror("------- Error in sending -------\n");
            return 1;
        }
        printf("------- Read & Sent %d bytes -------\n", sendlen);
        Trasmission++;
        // printf("MSG : \n%s\n",buf+1);
        for(int i=0;i<512;i++) buf[i]='\0';
        buf[0]='0';
        seq=(seq+1)%16;
    }

    // ------- SENDING "EOF" BLOCK ------- //
    buf[0]='~';
    buf[1]='\0';
    int sendlen;
    while(1)
    {
        while((sendlen=k_sendto(sockfd, buf, 1, 0, (struct sockaddr*)&Destination, sizeof(Destination)))<0 && errno==ENOSPACE) sleep(1);
        if(sendlen>=0) 
        {
            printf("Sent EOF as '~'\n");
            break;
        }
        if(errno==ENOTBOUND) printf("-------Error : ENOTBOUND-------");
        else perror("------- Error in sending -------\n");
        return 1;
    }
    printf("no. of messages generated from the file : %d\n",Trasmission);
    close(fd);
    sleep(10);    //WAIT FOL ALL THE LAST MESSAGES RECIVED BY THE RECIVER
    
    // ------- CLOSING BLOCK ------- //
    if (k_close(sockfd) < 0) 
    {
        perror("-------Error in closing-------\n Error ");
        exit(EXIT_FAILURE);
    }

    return 0;
}
