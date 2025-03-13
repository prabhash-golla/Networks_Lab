//==============================
// Assignment 5 Submission
// Name: <Your_Name>
// Roll number: <Your_Roll_Number>
//==============================
// Task Queue Server (Non-blocking I/O with fcntl and O_NONBLOCK)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <semaphore.h>


#define PORT 8080
#define MAX_TASKS 50
#define BUFFER_SIZE 64

struct sembuf pop, vop;

#define P(s) semop(s, &pop, 1) 
#define V(s) semop(s, &vop, 1) 


typedef struct 
{
    char tasks[MAX_TASKS][BUFFER_SIZE];
    int done[MAX_TASKS];
    // 0 -> Loaded ;
    // 1 -> Given to a Child but no response Till Now;
    // 2 -> Evaluated
    int task_count;
    int task_index;
    int Completed;
} MemType;

int shmid,Sem;
MemType* Mem;

void load_tasks(const char *filename) 
{
    FILE *file = fopen(filename, "r");
    P(Sem);
    if (!file) 
    {
        perror("Error opening task file");
        V(Sem);
        exit(EXIT_FAILURE);
    }
    while (fgets(Mem->tasks[Mem->task_count], BUFFER_SIZE, file) && Mem->task_count < MAX_TASKS) 
    {
        Mem->tasks[Mem->task_count][strcspn(Mem->tasks[Mem->task_count], "\n")] = 0; 
        Mem->done[Mem->task_count] = 0;
        Mem->task_count=Mem->task_count+1;
    }
    Mem->Completed = 0;
    V(Sem);
    fclose(file);
}

void sigchld_handler(int signo) 
{
    (void)signo; 
    // while (waitpid(-1, NULL, WNOHANG) > 0);
}

void handle_client(int client_fd) 
{
    char buffer[BUFFER_SIZE];
    while(1)
    {
        for(int i=0;i<BUFFER_SIZE;i++) buffer[i]='\0';
        int p = 5;
        int index = -1;
        while(p--)
        {
            read(client_fd, buffer, BUFFER_SIZE);
            // printf("Recived %s\n",buffer);
            if (strcmp(buffer, "GET_TASK") == 0) 
            {
                P(Sem);
                if (Mem->Completed==0 && Mem->done[Mem->task_index]==0) 
                {
                    char task_response[BUFFER_SIZE+6];
                    printf("Sending task : %s\n",Mem->tasks[Mem->task_index]);
                    snprintf(task_response, BUFFER_SIZE+6, "Task: %s", Mem->tasks[Mem->task_index]);
                    int present = Mem->task_index,i;
                    Mem->done[present]=1;
                    index  = present;
                    for(i=0;i < Mem->task_count;i++)
                    {
                        if(Mem->done[present]) present = (present+1)%Mem->task_count;
                        else break;
                    }
                    if(i==Mem->task_count) 
                    { 
                        printf("END of all Elements In The Queue\n");
                        Mem->Completed=1;
                    }
                    else 
                    {
                        Mem->task_index = present;
                    }
                    write(client_fd, task_response, strlen(task_response));
                } 
                else 
                {
                    write(client_fd, "No tasks available", 18);
                }
                V(Sem);
                break;
            } 
            else if (strncmp(buffer, "exit",4) == 0) 
            {

                printf("Recived \"EXIT\"\nClient disconnected.\n");
                close(client_fd);
                exit(EXIT_SUCCESS);
            }
            sleep(1);
        }
        if(p==-1)
        {
            printf("Client disconnected Since No message in 5 Sec\n");
            close(client_fd);
            exit(EXIT_FAILURE);
        }

        p = 5;
        
        while(p--)
        {
            sleep(1);
            read(client_fd, buffer, BUFFER_SIZE);
            if (strncmp(buffer, "RESULT", 6) == 0) 
            {
                printf("Received : %s\n", buffer);
                printf("M[%d] : %s\n",index,buffer+7);
                break;
            } 
            else if (strncmp(buffer, "exit",4) == 0) 
            {

                printf("Recived \"EXIT\"\nClient disconnected.\n");
                close(client_fd);
                exit(EXIT_SUCCESS);
            }
        }
        if(p==-1)
        {
            P(Sem);
            Mem->Completed=0;
            Mem->done[index]=0;
            V(Sem);
            printf("Client disconnected Since No message in 5 Sec\n");
            close(client_fd);
            exit(EXIT_FAILURE);
        }
    } 
}

// ------- HANDLE CTRL+C REMOVE THE SM AND SEMOPHERS ------- //
void sigHandler(int sig) 
{
    shmdt(Mem);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(Sem, 0, IPC_RMID);
    printf("\n-------Successfully Removed the Shared Memory-------\n");
    exit(EXIT_SUCCESS);
}

int main() 
{

    shmid = shmget(ftok("/",'P'),sizeof(MemType),0777|IPC_CREAT|IPC_EXCL);
    Mem = (MemType*) shmat(shmid,NULL,0);
    Sem = semget(ftok("/",'Q'), 1, 0777|IPC_CREAT);

    pop.sem_num = 0;
    vop.sem_num = 0;
	pop.sem_flg = 0;
    vop.sem_flg = 0;
	pop.sem_op = -1;
    vop.sem_op  = 1;

    semctl(Sem, 0, SETVAL, 1);

    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    signal(SIGCHLD, sigchld_handler);
    signal(SIGINT, sigHandler);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) 
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_fd, 5);

    printf("Server listening on port %d...\n", PORT);

    load_tasks("tasks.txt");

    while (1) 
    {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd < 0) continue;
        fcntl(client_fd, F_SETFL, O_NONBLOCK);

        if (fork() == 0) 
        {
            close(server_fd);
            handle_client(client_fd);
        }
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
