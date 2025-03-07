/*
=====================================
Assignment 4 Submission
Name: Golla Meghanandh Manvith Prabahash
Roll number: 22CS30027
=====================================
*/
#include "ksocket.h"
#include <sys/sem.h>
#include <assert.h>

struct SM_entry *SM;
struct sembuf pop, vop;

// pthread_mutex_t mutex_SM = PTHREAD_MUTEX_INITIALIZER;
// pthread_mutex_t mutex_SI = PTHREAD_MUTEX_INITIALIZER; 

int dummysem1,dummysem2,sem_SM;


// ------- SOCKET CREATION FUNCTION ------- //
int k_socket(int domain,int type,int protocol)
{
    // printf("-------Entered k_socket-------\n");

    // ------- SHARED MEMORY AND SEMOPHERS READING BLOCK ------- //
    int shmid = shmget(ftok("/",'P'),N*sizeof(struct SM_entry),0777);
    int dummysem1 = semget(ftok("/",'Q'), 1, 0777);
    int dummysem2 = semget(ftok("/",'R'), 1, 0777);
    int sem_SM = semget(ftok("/",'S'), 1, 0777);
    if(shmid == -1 || dummysem1 == -1 || dummysem2 == -1 || sem_SM == -1)
    {
        perror("----------error----------\nShared Memory ");
        exit(EXIT_FAILURE);
    }
    SM = (struct SM_entry*)shmat(shmid,0,0);

    pop.sem_num = 0;
    vop.sem_num = 0;
	pop.sem_flg = 0;
    vop.sem_flg = 0;
	pop.sem_op = -1; 
    vop.sem_op = 1;
    if(type!=SOCK_KTP)
    {
        errno = EINVAL;
        return -1;
    }

    // ------- FINDING FREE PLACE IN SM BLOCK------- //
    P(sem_SM);
    int k = -1;
    // pthread_mutex_lock(&mutex_SM);
    for(int i=0;i<N;i++)
    {
        if(SM[i].is_free)
        {
            SM[i].is_free=0;
            k = i;
            break;
        }
    }
    // pthread_mutex_unlock(&mutex_SM);
    if(k==-1)
    {
        // pthread_mutex_lock(&mutex_SI);
        errno = ENOSPACE;
        shmdt(SM);
        return -1;
        // pthread_mutex_unlock(&mutex_SI);
    }

    // ------- CREATEING SOCKET BLOCK------- //
    SM[k].do_crt=1;
    V(dummysem1);
    V(sem_SM);
    // pthread_mutex_lock(&mutex_SI);
    P(dummysem2);
    P(sem_SM);
    if(SM[k].sid==-1)
    {
        errno = SM[k].err_no;
        // pthread_mutex_unlock(&mutex_SI);
        shmdt(SM);
        return -1;
    }

    // pthread_mutex_unlock(&mutex_SI);
    // pthread_mutex_lock(&mutex_SM);

    // ------- INITILAIZING BLOCK ------- //
    SM[k].pid=getpid();
    for (int j = 0; j < 256; j++) 
    {
        SM[k].swnd.wndw[j] = -1;
        SM[k].lastSendTime[j] = -1;
        if (j>=1 && j < 11) SM[k].rwnd.wndw[j] = j-1;
        else SM[k].rwnd.wndw[j] = -1;
    }
    SM[k].swnd.size = 10;
    SM[k].rwnd.size = 10;
    SM[k].swnd.start_seq = 1;
    SM[k].rwnd.start_seq = 1;
    SM[k].sb_sz=10;
    for (int j = 0; j < 10; j++) SM[k].rb_valid[j] = 0;
    SM[k].rb_pointer=0;
    SM[k].nospace=0;
    SM[k].is_free=0;
    // pthread_mutex_unlock(&mutex_SM);
    // pthread_mutex_lock(&mutex_SI);

    V(sem_SM);
    shmdt(SM);

    // printf("-------exit k_socket successfully-------\n");
    return k;
}

// ------- SEND FUNCTION ------- //
int k_sendto(int sockid,const void *buff,size_t len,int flags,const struct sockaddr *destination,socklen_t addrlen)
{
    // printf("-------Entered k_sendto-------\n");

    // ------- SHARED MEMORY AND SEMOPHERS READING BLOCK ------- //
    int shmid = shmget(ftok("/",'P'),N*sizeof(struct SM_entry),0777);
    // int dummysem1 = semget(ftok("/",'Q'), 1, 0777);
    // int dummysem2 = semget(ftok("/",'R'), 1, 0777);
    int sem_SM = semget(ftok("/",'S'), 1, 0777);
    if(shmid ==-1 || sem_SM == -1)
    {
        perror("----------error----------\nShared Memory ");
        exit(EXIT_FAILURE);
    }
    SM = (struct SM_entry*)shmat(shmid,0,0);
    pop.sem_num = 0;
    vop.sem_num = 0;
	pop.sem_flg = 0;
    vop.sem_flg = 0;
	pop.sem_op = -1; 
    vop.sem_op = 1;

    if(sockid<0 || sockid>=N)
    {
        errno = EBADF;
        shmdt(SM);
        return -1;
    }

    char * dest_ip;
    uint16_t dest_port;
    dest_ip = inet_ntoa(((struct sockaddr_in *)destination)->sin_addr);
    dest_port = ntohs(((struct sockaddr_in *)destination)->sin_port);

    // ------- COMPARING THE BIND DESTINATION AND SEND DESTINATION ------- //
    P(sem_SM);
    if(strcmp(SM[sockid].ip_address, dest_ip)!=0 || SM[sockid].port!=dest_port)
    {
        errno = ENOTBOUND;
        V(sem_SM);
        shmdt(SM);
        return -1;
    }

    // ------- NO SPACE IN SEND BUFFER ------- //
    if(SM[sockid].sb_sz==0)
    {
        errno = ENOSPACE;
        V(sem_SM);  
        shmdt(SM);
        return -1;
    }
    int seq_no=SM[sockid].swnd.start_seq,buff_index,f;
    while(SM[sockid].swnd.wndw[seq_no]!=-1) seq_no=(seq_no+1)%256;
    for(buff_index=0;buff_index<10;buff_index++)
    {
        f=1;
        for(int i=0;i<256;i++)
        {
            if(SM[sockid].swnd.wndw[i]==buff_index)
            {
                f=0;
                break;
            }
        }
        if(f==1) break;
    }
    if(f==0)
    {
        errno=ENOSPACE;
        V(sem_SM);
        shmdt(SM);
        return -1;
    }

    // ------- STOREING IN SEND BUFFER ------- //
    SM[sockid].swnd.wndw[seq_no]=buff_index;              
    memcpy(SM[sockid].sb[buff_index], buff, len);
    SM[sockid].lastSendTime[seq_no]=-1;
    SM[sockid].sb_sz--;
    SM[sockid].lens[buff_index]=len;
    V(sem_SM);
    shmdt(SM);

    // printf("-------Exit k_sendto successfully-------\n");
    return len;
}

// ------- RECIEVE FUNCTION ------- //
int k_recvfrom(int sockid,void *buff,size_t len,int flag,struct sockaddr *source,socklen_t *addrlen)
{
    // printf("-------Entered k_recvfrom-------\n");

    // ------- SHARED MEMORY AND SEMOPHERS READING BLOCK ------- //
    int shmid = shmget(ftok("/",'P'),N*sizeof(struct SM_entry),0777);
    // int dummysem1 = semget(ftok("/",'Q'), 1, 0777);
    // int dummysem2 = semget(ftok("/",'R'), 1, 0777);
    int sem_SM = semget(ftok("/",'S'), 1, 0777);
    if(shmid ==-1 || sem_SM == -1)
    {
        perror("----------error----------\nShared Memory ");
        exit(EXIT_FAILURE);
    }
    SM = (struct SM_entry*)shmat(shmid,0,0);
    pop.sem_num = 0;
    vop.sem_num = 0;
	pop.sem_flg = 0;
    vop.sem_flg = 0;
	pop.sem_op = -1; 
    vop.sem_op = 1;

    P(sem_SM);
    if(sockid<0 || sockid>=N || SM[sockid].is_free)
    {
        errno = EBADF;
        shmdt(SM);
        return -1;
    }

    // ------- READING FROM RECIVE BUFFER ------- //
    if (SM[sockid].rb_valid[SM[sockid].rb_pointer]) 
    {
        SM[sockid].rb_valid[SM[sockid].rb_pointer] = 0;
        SM[sockid].rwnd.size++;
        int seq = -1;
        for (int i = 0; i < 256; i++) if (SM[sockid].rwnd.wndw[i] == SM[sockid].rb_pointer) seq = i;
        SM[sockid].rwnd.wndw[seq] = -1;
        SM[sockid].rwnd.wndw[(seq+10)%256] = SM[sockid].rb_pointer;
        int length = SM[sockid].lenr[SM[sockid].rb_pointer];
        memcpy(buff, SM[sockid].rb[SM[sockid].rb_pointer], (len < length) ? len : length);
        SM[sockid].rb_pointer = (SM[sockid].rb_pointer + 1) % 10;

        struct sockaddr_in *src_addr = (struct sockaddr_in *)source;
        src_addr->sin_family = AF_INET;
        src_addr->sin_port = htons(SM[sockid].port); 
        src_addr->sin_addr.s_addr =  inet_addr(SM[sockid].ip_address);
        *addrlen = sizeof(struct sockaddr_in);
        
        V(sem_SM);
        shmdt(SM);
        // printf("-------Exit k_recvfrom successfully-------\n");
        return (len < length) ? len : length;
    }

    // ------- NO MESSAGE NOTIFING BLOCK ------- //
    errno = ENOMESSAGE;
    V(sem_SM);
    shmdt(SM);
    return -1;

}

// ------- BIND CREATION FUNCTION ------- //
int k_bind(int sockid,struct sockaddr *source,struct sockaddr *destination)
{
    // printf("-------Entered k_bind-------\n",sockid);

    // ------- SHARED MEMORY AND SEMOPHERS READING BLOCK ------- //
    int shmid = shmget(ftok("/",'P'),N*sizeof(struct SM_entry),0777);
    dummysem1 = semget(ftok("/",'Q'), 1, 0777);
    dummysem2 = semget(ftok("/",'R'), 1, 0777);
    sem_SM = semget(ftok("/",'S'), 1, 0777);
    if(shmid ==-1 || dummysem1 == -1 || dummysem2  == -1 || sem_SM == -1)
    {
        perror("----------error----------\nShared Memory ");
        exit(EXIT_FAILURE);
    }
    SM = (struct SM_entry*)shmat(shmid,0,0);
    pop.sem_num = 0;
    vop.sem_num = 0;
	pop.sem_flg = 0;
    vop.sem_flg = 0;
	pop.sem_op = -1; 
    vop.sem_op = 1;

    if(sockid<0 || sockid>=N)
    {
        errno = EBADF;
        shmdt(SM);
        return -1;
    }

    struct sockaddr_in *a = (struct sockaddr_in *)source;
    char ip[INET_ADDRSTRLEN]; 
    P(sem_SM);

    // ------- STORING SELF IP AND PORT------- //
    inet_ntop(AF_INET, &(a->sin_addr), ip, INET_ADDRSTRLEN); 
    strncpy(SM[sockid].self_ip_address, ip,sizeof(SM[sockid].self_ip_address)-1); 
    SM[sockid].self_port = a->sin_port;

    // ------- BINDING ------- //
    SM[sockid].do_bind=1;
    V(dummysem1);
    V(sem_SM);

    P(dummysem2);
    P(sem_SM);
    if(SM[sockid].sid==-1)
    {
        errno = SM[sockid].err_no;
        V(sem_SM);
        shmdt(SM);
        return -1;
    }

    // ------- STORING OTHER'S IP AND PORT------- //
    struct sockaddr_in *b = (struct sockaddr_in *)destination;
    inet_ntop(AF_INET, &(b->sin_addr), ip, INET_ADDRSTRLEN); 
    strncpy(SM[sockid].ip_address, ip, sizeof(SM[sockid].ip_address)- 1); 
    SM[sockid].ip_address[sizeof(SM[sockid].ip_address) - 1] = '\0'; 
    SM[sockid].port = ntohs(((struct sockaddr_in *)destination)->sin_port);
    V(sem_SM);
    shmdt(SM);

    // printf("-------Exit k_bind successfully-------");
    return 0;
}

// ------- SOCKET CLOSE FUNCTION ------- //
int k_close(int sockid)
{
    // printf("-------Entered k_close-------\n");

    // ------- SHARED MEMORY AND SEMOPHERS READING BLOCK ------- //
    int shmid = shmget(ftok("/",'P'),N*sizeof(struct SM_entry),0777);
    int sem_SM = semget(ftok("/",'S'), 1, 0777);
    if(shmid ==-1 || sem_SM == -1)
    {
        perror("-------error-------\nShared Memory ");
        exit(EXIT_FAILURE);
    }
    SM = (struct SM_entry*)shmat(shmid,0,0);
    pop.sem_num = vop.sem_num = 0;
	pop.sem_flg = vop.sem_flg = 0;
	pop.sem_op = -1; vop.sem_op = 1;

    // ------- SET FREE IN SM ------- //
    P(sem_SM);
    SM[sockid].is_free=1;
    V(sem_SM);
    shmdt(SM);

    // printf("-------Exit k_close-------");
    return 0;
}

// ------- DROP A MESSAGE FUNCTION FUNCTION ------- //
int dropMessage() 
{
    return (double)rand() / RAND_MAX < p;
}