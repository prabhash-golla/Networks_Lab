//==============================
// Assignment 5 Submission
// Name: Golla Meghanandh Manvith Prabhash
// Roll number: 22CS30027
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
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <semaphore.h>


#define PORT 8080
#define MAX_TASKS 50
#define BUFFER_SIZE 128
#define MAX_GET_TASK 4

struct sembuf pop, vop;

#define P(s) semop(s, &pop, 1) 
#define V(s) semop(s, &vop, 1) 

char *filename;

// Task status codes - better terminology
#define TASK_PENDING 0    // Task is available for processing (initial or re-enqueued)
#define TASK_ASSIGNED 1   // Task is assigned to a client but not yet completed
#define TASK_COMPLETED 2  // Task has been successfully completed

// Queue node structure
typedef struct QueueNode 
{
    char task[BUFFER_SIZE];
    int status;  // 0: pending, 1: assigned, 2: completed
    int id;      // Unique task ID
    int attempt_count; // Number of times this task has been assigned
} QueueNode;

// Queue structure for shared memory
typedef struct 
{
    QueueNode tasks[MAX_TASKS];
    int front;
    int rear;
    int size;
    int next_id;
    int all_tasks_processed;
} TaskQueue;

int shmid, Sem;
TaskQueue* queue;

// Queue operations
void init_queue() 
{
    P(Sem);
    queue->front = 0;
    queue->rear = -1;
    queue->size = 0;
    queue->next_id = 0;
    queue->all_tasks_processed = 0;
    V(Sem);
}

int is_queue_empty() 
{
    P(Sem);
    int p = queue->size;
    V(Sem);
    return p == 0;
}

int is_queue_full() 
{
    P(Sem);
    int p = queue->size;
    V(Sem);
    return p == MAX_TASKS;
}

void enqueue(const char* task) 
{
    if (is_queue_full()) 
    {
        printf("Queue is full, cannot add more tasks\n");
        return;
    }
    
    P(Sem);
    queue->rear = (queue->rear + 1) % MAX_TASKS;
    strncpy(queue->tasks[queue->rear].task, task, BUFFER_SIZE - 1);
    queue->tasks[queue->rear].task[BUFFER_SIZE - 1] = '\0';
    queue->tasks[queue->rear].status = TASK_PENDING;
    queue->tasks[queue->rear].id = queue->next_id++;
    queue->tasks[queue->rear].attempt_count = 0;
    queue->size++;
    V(Sem);
}

int dequeue_task(char* task_buffer, int* task_id) 
{
    if (is_queue_empty()) return -1;
    
    P(Sem);
    // Find first pending task
    int current = queue->front;
    int found = 0;
    int count = 0;
    
    while (count < queue->size && !found) 
    {
        if (queue->tasks[current].status == TASK_PENDING) 
        {
            strncpy(task_buffer, queue->tasks[current].task, BUFFER_SIZE);
            *task_id = queue->tasks[current].id;
            queue->tasks[current].status = TASK_ASSIGNED;
            queue->tasks[current].attempt_count++;
            found = 1;
            
            printf("Assigned task ID %d (attempt #%d): %s\n", *task_id, queue->tasks[current].attempt_count,queue->tasks[current].task);
        }
        current = (current + 1) % MAX_TASKS;
        count++;
    }
    
    // Check if all tasks are assigned or completed
    int all_processed = 1;
    current = queue->front;
    count = 0;
    
    while (count < queue->size) 
    {
        if (queue->tasks[current].status == TASK_PENDING) 
        {
            all_processed = 0;
            break;
        }
        current = (current + 1) % MAX_TASKS;
        count++;
    }
    
    queue->all_tasks_processed = all_processed;
    V(Sem);
    
    return found ? 0 : -1;
}

void update_task_result(int task_id, const char* result) 
{
    P(Sem);
    int current = queue->front;
    int count = 0;
    
    while (count < queue->size) 
    {
        if (queue->tasks[current].id == task_id) 
        {
            char new_result[BUFFER_SIZE*2];
            snprintf(new_result, BUFFER_SIZE*2, "%s = %s", queue->tasks[current].task, result);
            strncpy(queue->tasks[current].task, new_result, BUFFER_SIZE - 1);
            queue->tasks[current].task[BUFFER_SIZE - 1] = '\0';
            queue->tasks[current].status = TASK_COMPLETED;
            printf("Task ID %d completed after %d attempts\n", 
                   task_id, 
                   queue->tasks[current].attempt_count);
            break;
        }
        current = (current + 1) % MAX_TASKS;
        count++;
    }
    V(Sem);
}

void requeue_task(int task_id) 
{
    P(Sem);
    int current = queue->front;
    int count = 0;
    
    while (count < queue->size) 
    {
        if (queue->tasks[current].id == task_id) 
        {
            queue->tasks[current].status = TASK_PENDING;
            queue->all_tasks_processed = 0;
            printf("Re-enqueueing task ID %d: %s (attempt #%d failed)\n", 
                   task_id, 
                   queue->tasks[current].task,
                   queue->tasks[current].attempt_count);
            break;
        }
        current = (current + 1) % MAX_TASKS;
        count++;
    }
    V(Sem);
}

void load_tasks(const char *filename) 
{
    int fd = open(filename, O_RDONLY);
    if (fd == -1) 
    {
        perror("Error opening task file");
        exit(EXIT_FAILURE);
    }
    
    init_queue();
    
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buffer, sizeof(buffer) - 1)) > 0) 
    {
        buffer[bytes_read] = '\0';
        char *token = strtok(buffer, "\n");
        while (token && !is_queue_full()) 
        {
            enqueue(token);
            token = strtok(NULL, "\n");
        }
    }
    
    close(fd);
}

void write_results(const char *filename) 
{
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) 
    {
        perror("Error opening task file for writing");
        exit(EXIT_FAILURE);
    }
    
    P(Sem);
    int current = queue->front;
    int count = 0;
    
    while (count < queue->size) 
    {
        char task_line[BUFFER_SIZE + 10];
        snprintf(task_line, sizeof(task_line), "%s\n", queue->tasks[current].task);
        write(fd, task_line, strlen(task_line));
        current = (current + 1) % MAX_TASKS;
        count++;
    }
    V(Sem);
    
    close(fd);
}

void sigchld_handler(int signo) 
{
    (void)signo; 
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

void handle_client(int client_fd) 
{
    signal(SIGINT, SIG_DFL);
    char buffer[BUFFER_SIZE];
    int task_id = -1;
    char task_content[BUFFER_SIZE];
    int num_get_task = 0;
    
    while(1) 
    {
        num_get_task = 0;
        memset(buffer, 0, BUFFER_SIZE);
        int timeout_counter = 5;
        int result=-1;
        int got_response = 0;
        
        // Wait for GET_TASK request
        while(timeout_counter-- > 0) 
        {
            if (read(client_fd, buffer, BUFFER_SIZE) > 0) 
            {
                got_response = 1;
                break;
            }
            sleep(1);
        }
        
        if (!got_response) 
        {
            printf("Client timed out waiting for request\n");
            if (task_id >= 0) 
            {
                requeue_task(task_id);  // Re-enqueue the previously assigned task
                task_id = -1;
            }
            close(client_fd);
            exit(EXIT_FAILURE);
        }
        
        if (strcmp(buffer, "GET_TASK") == 0) 
        {
            // If there was a previous task that didn't get a result, re-enqueue it first
            if (task_id >= 0) 
            {
                requeue_task(task_id);
                task_id = -1;
            }
            num_get_task++;
            // Get a new task
            memset(task_content, 0, BUFFER_SIZE);
            result = dequeue_task(task_content, &task_id);
            
            if (result == 0) 
            {
                char task_response[BUFFER_SIZE + 6];
                snprintf(task_response, BUFFER_SIZE + 6, "Task: %s", task_content);
                write(client_fd, task_response, strlen(task_response));
            } 
            else 
            {
                write(client_fd, "No tasks available", 18);
                task_id = -1;
            }
        } 
        else if (strncmp(buffer, "exit", 4) == 0) 
        {
            printf("Received \"EXIT\"\nClient disconnected.\n");
            // Re-enqueue any task that was assigned but not completed
            if (task_id >= 0) requeue_task(task_id);
            close(client_fd);
            exit(EXIT_SUCCESS);
        } 
        else continue;
        
        // If no task was assigned, continue to next iteration
        if (task_id < 0) continue;
        
        // Wait for RESULT response
        timeout_counter = 5;
        got_response = 0;
        
        while(timeout_counter-- > 0 && !got_response) 
        {
            sleep(1);
            memset(buffer, 0, BUFFER_SIZE);
            
            if (read(client_fd, buffer, BUFFER_SIZE) > 0) 
            {
                // printf("Buffer : %s\n",buffer);
                got_response = 1;
                if (strncmp(buffer, "RESULT", 6) == 0) 
                {
                    if (task_id >= 0) 
                    {
                        update_task_result(task_id, buffer + 7);
                        write_results("tasks.txt");
                        task_id = -1; // Clear the task ID after processing
                    }
                } 
                else if (strncmp(buffer, "exit", 4) == 0) 
                {
                    printf("Received \"EXIT\"\nClient disconnected.\n");
                    // Re-enqueue any task that was assigned but not completed
                    if (task_id >= 0) requeue_task(task_id);
                    close(client_fd);
                    exit(EXIT_SUCCESS);
                } 
                else if (strcmp(buffer, "GET_TASK") == 0) 
                {
                    if(num_get_task!=MAX_GET_TASK) 
                    {
                        num_get_task++;
                        if (result == 0) 
                        {
                            char task_response[BUFFER_SIZE + 6];
                            snprintf(task_response, BUFFER_SIZE + 6, "Task: %s", task_content);
                            printf("Send Again : %s\n",task_response);
                            write(client_fd, task_response, strlen(task_response));
                        } 
                        else 
                        {
                            write(client_fd, "No tasks available", 18);
                            task_id = -1;
                        }
                        timeout_counter = 5;
                        got_response = 0;
                        continue;
                    }
                }
                else 
                {
                    // Invalid response, re-enqueue the task
                    if (task_id >= 0) 
                    {
                        requeue_task(task_id);
                        task_id = -1;
                    }
                }
            }
        }
        
        if (!got_response) 
        {
            printf("Client timed out waiting for result\n");
            if (task_id >= 0) {
                printf("Re-enqueueing task due to no result received\n");
                requeue_task(task_id);
                task_id = -1;
            }
            close(client_fd);
            exit(EXIT_FAILURE);
        }
        
        
    }
}

// ------- HANDLE CTRL+C REMOVE THE SM AND SEMOPHERS ------- //

// Global flag to track if cleanup has been done
volatile sig_atomic_t cleanup_done = 0;

void sigHandler(int sig) 
{
    // Only perform cleanup once
    if (!cleanup_done) 
    {
        cleanup_done = 1;
        
        shmdt(queue);
        shmctl(shmid, IPC_RMID, NULL);
        semctl(Sem, 0, IPC_RMID);
        printf("\n-------Successfully Removed the Shared Memory-------\n");
    }
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[]) 
{
    if (argc < 2)
    {
        printf("Missing File Name. Usage: %s <file_name>\n", argv[0]);
        exit(EXIT_FAILURE); 
    }
    else
    {
        filename = argv[1];
    }

    shmid = shmget(IPC_PRIVATE, sizeof(TaskQueue), 0777 | IPC_CREAT | IPC_EXCL);
    Sem = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
    
    if(shmid==-1 || Sem == -1)
    {
        printf("Shared Mem / Sem Creation Error\n");
        exit(EXIT_FAILURE);
    }
    
    queue = (TaskQueue*) shmat(shmid, NULL, 0);
    pop.sem_num = 0;
    vop.sem_num = 0;
    pop.sem_flg = 0;
    vop.sem_flg = 0;
    pop.sem_op = -1;
    vop.sem_op = 1;
    
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
    
    int b = bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (b < 0) 
    {
        perror("Binding failed");
        exit(EXIT_FAILURE);
    }

    listen(server_fd, 5);
    
    printf("Server listening on port %d...\n", PORT);
    
    load_tasks(filename);
    
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