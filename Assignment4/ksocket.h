/*
=====================================
Assignment 4 Submission
Name: Golla Meghanandh Manvith Prabahash
Roll number: 22CS30027
=====================================
*/

#ifndef KSOCKET_H
#define KSOCKET_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <pthread.h>
#include <stdint.h>
#include <semaphore.h>


// ------- GLOBAL CONSTANTS ------- //
#define SOCK_KTP 5
#define T 5
#define p 0.05
#define N 5

// ------- ERROR NUMBER DEFINATION ------- //
#define ENOSPACE 1000
#define ENOTBOUND 1001
#define ENOMESSAGE 1002

// ------- WINDOW STRUCTURE ------- //
struct window 
{
    int size;
    int start_seq;
    int wndw[256];
};

// ------- SHARED MEMORY STRUCTURE ------- //
struct SM_entry 
{
    int is_free;               
    pid_t pid;          
    int sid;   
    char self_ip_address[16];       
    uint16_t self_port;     
    char ip_address[16];       
    uint16_t port;             
    char sb[10][512];    
    int sb_sz;
    int lens[10];
    char rb[10][512];    
    int rb_pointer;   
    int rb_valid[10];  
    int lenr[10];
    struct window swnd;        
    struct window rwnd; 
    int nospace;    
    int do_bind;
    int do_crt;  
    int err_no;         
    time_t lastSendTime[256];    
    // pthread_mutex_t lock;
};


// ------- GLOBAL VARIABLES ------- //
extern struct sembuf pop, vop;
extern struct SM_entry *SM;
extern int dummysem1,dummysem2,sem_SM;
#define P(s) semop(s, &pop, 1) 
#define V(s) semop(s, &vop, 1) 

// ------- FUNCTION PROTOTYPES ------- //
int k_socket(int domain,int type,int protocol);
int k_sendto(int sockid,const void *buff,size_t len,int flags,const struct sockaddr *destination,socklen_t addrlen);
int k_recvfrom(int sockid,void *buff,size_t len,int flag,struct sockaddr *source,socklen_t *addrlen);
int k_bind(int sockid,struct sockaddr *source,struct sockaddr *destination);
int k_close(int sockid);
int dropMessage();

// Getter functions to access mutexes
// pthread_mutex_t mutex_SM,mutex_SI;
// pthread_mutex_t* get_mutex_SM() { return &mutex_SM; }
// pthread_mutex_t* get_mutex_SI() { return &mutex_SI; }

#endif
