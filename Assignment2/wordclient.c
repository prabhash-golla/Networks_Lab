/*
=====================================
Assignment 2 Submission
Name: Golla Meghanandh Manvith Prabahash
Roll number: 22CS30027
Link of the pcap file: https://drive.google.com/file/d/15ajm7OUr6qgsK-CWGxivl2tbwIQdHW9c/view?usp=sharing
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

const int maxlen=1000;

int main()
{
    char* buffer;
    buffer=(char*)malloc(maxlen*sizeof(char));
    int sockid;
    struct sockaddr_in serveraddr,clientaddr;
    sockid=socket(AF_INET,SOCK_DGRAM,0);
    if(!sockid)
    {
        perror("Socket creation failed: ");
        exit(EXIT_FAILURE);
    }
    printf("Socket created successfully for Client %d\n",sockid);
    serveraddr.sin_family=AF_INET;
    serveraddr.sin_addr.s_addr=INADDR_ANY;
    serveraddr.sin_port=htons(1308);
    clientaddr.sin_family=AF_INET;
    clientaddr.sin_addr.s_addr=INADDR_ANY;
    clientaddr.sin_port=htons(1309);
    // if(!bind(sockid,(struct sockaddr*)&serveraddr,sizeof(serveraddr))<0)
    // {
    //     perror("Bind failed: ");
    //     exit(EXIT_FAILURE);
    // }
    // printf("Client Bind successful\n");
    char filename[100];
    printf("Enter file name : ");
    scanf("%s",filename);
    printf("Entered File Name : %s\n",filename);
    sendto(sockid,filename,strlen(filename),0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
    int serverlen=sizeof(serveraddr);
    // while(1)
    // {
        int len=recvfrom(sockid,buffer,maxlen,MSG_WAITALL,(struct sockaddr*)&serveraddr,&serverlen);
        buffer[len]='\0';
        char temp[1100];
        strcpy(temp,"NOTFOUND ");
        strcat(temp,filename);
        if(strcmp(buffer,temp)==0)
        {
            printf("FILE NOT FOUND\n");
            exit(0);
        }
        if(strcmp(buffer,filename)==0)
        {
            printf("SERVER NOT CONNECTED\n");
            exit(0);
        }
        printf("File found on server, starting receiving\n");
        int i=1;
        char RecFileName[1100];
        strcpy(RecFileName,filename);
        strcat(RecFileName,"(copy).txt");
        FILE*fpointer=fopen(RecFileName, "w");
        if(strcmp(buffer,"HELLO\0")!=0)
        {
            printf("The First Word in the File is not HELLO\n");
            printf("Word received: %s",buffer);
            exit(EXIT_FAILURE);
        }
        while(strcmp(buffer,"FINISH")!=0)
        {
            fprintf(fpointer,"%s\n",buffer);
            printf("Word received: %s\n",buffer);
            char temp[100]="WORD";
            char RecFileName[100];
            sprintf(RecFileName,"%d",i);
            strcat(temp,RecFileName);
            i++;
            printf("Sending: %s\n",temp);
            sendto(sockid,temp,strlen(temp),0,(struct sockaddr*)&serveraddr,sizeof(serveraddr));
            int len=recvfrom(sockid,buffer,maxlen,0,(struct sockaddr*)&serveraddr,&serverlen);
            buffer[len]='\0';
        }
        fprintf(fpointer,"%s",buffer);
        printf("Word received: %s\n",buffer);
        printf("File received successfully\n");
        fclose(fpointer);
        exit(0);
    // }
        
}