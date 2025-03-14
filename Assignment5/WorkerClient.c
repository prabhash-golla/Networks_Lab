//==============================
// Assignment 5 Submission
// Name: Golla Meghanandh Manvith Prabhash
// Roll number: 22CS30027
//==============================
// Worker Client Code - Improved Version

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

/*
 * Evaluates a mathematical expression string and returns the result
 * @param task String containing a mathematical expression (e.g., "5 + 3")
 * @return Result of the mathematical operation
 */
int evaluate_task(const char *task) 
{
    int a, b;
    char op;
    
    if (sscanf(task, "%d %c %d", &a, &op, &b) != 3) 
    {
        printf("Error: Invalid task format '%s'\n", task);
        return 0;
    }
    
    switch (op) 
    {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': 
            if (b == 0) 
            {
                printf("Warning: Division by zero detected\n");
                return 0;
            }
            return a / b;
        default: 
            printf("Error: Unknown operator '%c'\n", op);
            return 0;
    }
}


void print_terminal_line(char character) 
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    
    char line[w.ws_col + 1];
    memset(line, character, w.ws_col);
    line[w.ws_col] = '\0';
    
    printf("%s\n", line);
}

int main(int argc, char* argv[]) 
{
    // Set default number of tasks to process
    int tasks_to_process = 1;
    
    // Parse command line arguments
    if (argc == 2) 
    {
        tasks_to_process = atoi(argv[1]);
        if (tasks_to_process <= 0) 
        {
            printf("Invalid number of tasks. Using default (1).\n");
            tasks_to_process = 1;
        }
    }
    
    // Socket setup
    int client_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    
    // Create socket
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) 
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    
    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    // Use loopback address for local testing
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) 
    {
        perror("Invalid address");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    // Connect to server
    printf("Attempting to connect to server...\n");
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) 
    {
        perror("Connection failed");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    printf("Connected to server successfully.\n");
    
    // Process tasks
    int tasks_completed = 0;
    
    while (tasks_completed < tasks_to_process) 
    {
        // Clear buffer before each operation
        memset(buffer, 0, BUFFER_SIZE);
        
        // Request a task
        print_terminal_line('_');
        printf("\nRequesting task from server...\n");
        if (write(client_fd, "GET_TASK", 8) < 0) 
        {
            perror("Failed to send GET_TASK");
            break;
        }
        
        // Receive task from server
        ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read < 0) 
        {
            perror("Failed to receive task");
            break;
        } else if (bytes_read == 0) 
        {
            printf("Server closed connection\n");
            break;
        }
        
        buffer[bytes_read] = '\0';  // Ensure null termination
        
        // Process the received task
        if (strncmp(buffer, "Task:", 5) == 0) 
        {
            printf("Received task: %s\n", buffer + 5);
            
            // Evaluate the mathematical expression
            int result = evaluate_task(buffer + 6);
            printf("Calculated result: %d\n", result);
            
            // Send result back to server
            char result_msg[BUFFER_SIZE];
            snprintf(result_msg, BUFFER_SIZE, "RESULT %d", result);
            printf("Sending result to server: %s\n", result_msg);
            
            if (write(client_fd, result_msg, strlen(result_msg)) < 0) 
            {
                perror("Failed to send result");
                break;
            }
            
            tasks_completed++;
            printf("Tasks completed: %d/%d\n", tasks_completed, tasks_to_process);
        }
        else if (strncmp(buffer, "No tasks available", 18) == 0) 
        {
            printf("No tasks available on server\n");
            break;
        }
        else {
            printf("Unexpected response from server: %s\n", buffer);
            break;
        }
        
        // Wait a bit before requesting next task
        sleep(1);
    }
    
    print_terminal_line('_');
    // Gracefully exit by informing the server
    printf("\nWork completed. Sending exit message to server...\n");
    if (write(client_fd, "exit", 4) < 0) 
    {
        perror("Failed to send exit message");
    }
    
    close(client_fd);
    printf("Disconnected from server.\n");
    
    return 0;
}