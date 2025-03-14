//==============================
// Assignment 5 Submission
// Name: <Your_Name>
// Roll number: <Your_Roll_Number>
//==============================
// Worker Client Code

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int evaluate_task(const char *task) 
{
    int a, b;
    char op;
    sscanf(task, "%d %c %d", &a, &op, &b);
    switch (op) 
    {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return b != 0 ? a / b : 0;
        default: return 0;
    }
}

int main(int argc,char* argv[]) 
{
    int k = 1;
    if(argc == 2)
    {
        k = atoi(argv[1]);
    }
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) 
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to server.\n");
    while (k--)
    {  
        printf("Sending GET_TASK\n");
        write(client_fd, "GET_TASK", 8);
    
        for(int i=0;i<1024;i++) buffer[i]='\0';
        read(client_fd, buffer, BUFFER_SIZE);
        if (strncmp(buffer, "Task:", 5) == 0) 
        {
            printf("Received task: %s\n", buffer + 5);
            // int result = evaluate_task(buffer + 6);
            // printf("Result : %d\n",result);
            // char result_msg[BUFFER_SIZE];
            // snprintf(result_msg, BUFFER_SIZE, "RESULT %d", result);
            // printf("Sending %s\n",result_msg);
            // write(client_fd, result_msg, strlen(result_msg));
        } 
        else if(strncmp(buffer,"No tasks available\n",19))
        {
            printf("Recived \"No tasks available\" \n");
            write(client_fd, "exit",4);
            break;
        }
        else 
        {
            printf("Others : %s\n", buffer);
            break;
        }
        sleep(1);
    }
    if(k==-1)
    {
        printf("Sending \"exit\" \n");
        write(client_fd, "exit",4);
    }
    

    close(client_fd);
    return 0;
}
