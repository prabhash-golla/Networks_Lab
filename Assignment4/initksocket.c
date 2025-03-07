/*
=====================================
Assignment 4 Submission
Name: Golla Meghanandh Manvith Prabahash
Roll number: 22CS30027
=====================================
*/

#include "ksocket.h"
#include <sys/sem.h>

// struct SM_entry *SM;
// ------- GLOBAL VARIABLES ------- //
int shmid,Transmitions;

// ------- HANDLE CTRL+C REMOVE THE SM AND SEMOPHERS ------- //
void sigHandler(int sig) 
{
    shmdt(SM);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(dummysem1, 0, IPC_RMID);
    semctl(dummysem2, 0, IPC_RMID);
    semctl(sem_SM, 0, IPC_RMID);
    printf("\n-------Successfully Removed the Shared Memory-------\n");
    exit(EXIT_SUCCESS);
}

// ------- RECIVER THREAD FUNCTION ------- //
void* R()
{
    fd_set ReadFds;
    FD_ZERO(&ReadFds);
    int maxfd = 0;

    while (1) 
    {
        fd_set readyfds = ReadFds;
        struct timeval time;
        time.tv_sec = T;
        time.tv_usec = 0;

        // printf("-------R Thread-------\n");

        // ------- SLELECT CALL ------- // 
        int retval = select(maxfd + 1, &readyfds, NULL, NULL, &time);
        if (retval == -1) 
        {
            perror("-------Error-------\nselect() ");
        }
        if (retval <= 0) 
        {
            FD_ZERO(&ReadFds);
            maxfd = 0;
            // pthread_mutex_lock(&mutex_SM);
            // ------- SEND ALL REMAINING WINDOW SIZES ------- // 
            P(sem_SM);
            for (int i = 0; i < N; i++) 
            {
                if (SM[i].is_free == 0) 
                {
                    FD_SET(SM[i].sid, &ReadFds);
                    if (SM[i].sid > maxfd) maxfd = SM[i].sid;

                    if (SM[i].nospace && SM[i].rwnd.size > 0) SM[i].nospace = 0;
                    int lastseq = (SM[i].rwnd.start_seq + 255) % 256;
                    struct sockaddr_in cliaddr;
                    cliaddr.sin_family = AF_INET;
                    cliaddr.sin_addr.s_addr = inet_addr(SM[i].ip_address);
                    cliaddr.sin_port = htons(SM[i].port);

                    char ack[14];
                    ack[0] = '0';
                    for (int j = 0; j < 8; j++) ack[8 - j] = ((lastseq >> j) & 1) + '0';
                    for (int j = 0; j < 4; j++) ack[12 - j] = ((SM[i].rwnd.size >> j) & 1) + '0';
                    ack[13] = '\0';
                    // printf("-------Send1 : %s-------\n",ack);   

                    int k = sendto(SM[i].sid, ack, 14, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
                    if(k<0) perror("Error Sending 1\n");
                }
            }
            // pthread_mutex_unlock(&mutex_SM);
            V(sem_SM);
        } 
        else 
        {
            // pthread_mutex_lock(&mutex_SM);
            P(sem_SM);
            for (int i = 0; i < N; i++) 
            {
                if (FD_ISSET(SM[i].sid, &readyfds)) 
                {
                    // printf("\n-------value 222:-  %d  socket %d seq : %d-------\n", SM[i].rwnd.size,i,SM[i].rwnd.start_seq);
                    char buffer[532];
                    struct sockaddr_in cliaddr;
                    socklen_t len = sizeof(cliaddr);

                    // ------- RECIEVE CALL ------- // 
                    // printf("-------Going to Recive-------\n");
                    int n = recvfrom(SM[i].sid, buffer, 532, 0, (struct sockaddr *)&cliaddr, &len);
                    if (n < 0) 
                    {
                        perror("recvfrom()");
                    } 
                    else 
                    {
                        // ------- ENSURE MESSAGE COMMING ONLY FROM REQUESTED PORTS ------- // 
                        // printf("SM[i].port : %d :: %d\n",SM[i].port,ntohs(cliaddr.sin_port));
                        if(ntohs(cliaddr.sin_port)!=SM[i].port) continue;
                        if (cliaddr.sin_addr.s_addr != inet_addr(SM[i].ip_address)) continue;
                        // if(n==20 && buffer[19]=='~') printf("Total number of transmissions that are made to send the message : %d\n",Transmitions-1);
                        // else 
                        // if (dropMessage()) continue;
                        // printf("-------Recv : %.*s-------\n", n, buffer);   
                        if (buffer[0] == '0') 
                        {
                            int seq = 0, rwnd = 0;
                            for (int j = 1; j <= 8; j++) seq = (seq << 1) | (buffer[j] - '0');  
                            for (int j = 9; j <= 12; j++) rwnd = (rwnd << 1) | (buffer[j] - '0');

                            // printf("-------Hellow SM[i].swnd.wndw[seq] %d ; seq %d-------\n",SM[i].swnd.wndw[seq],seq);
                            if (SM[i].swnd.wndw[seq] >= 0) 
                            {
                                int j = SM[i].swnd.start_seq;
                                // printf("-------j : %d ; seq : %d-------\n",j,seq);
                                while (j != (seq + 1) % 256) 
                                {
                                    SM[i].swnd.wndw[j] = -1;
                                    SM[i].lastSendTime[j] = -1;
                                    SM[i].sb_sz++;
                                    j = (j + 1) % 256;
                                }
                                SM[i].swnd.start_seq = (seq + 1) % 256;
                            }
                            SM[i].swnd.size = rwnd;
                        }
                        else 
                        { 
                            int seq = 0, len = 0;
                            for (int j = 1; j <= 8; j++) seq = (seq << 1) | (buffer[j] - '0');
                            for (int j = 9; j <= 18; j++) len = (len << 1) | (buffer[j] - '0');
                            // printf("-------value 333:-  %d  socket %d  seq : %d-------\n", SM[i].rwnd.size,i,SM[i].rwnd.start_seq);
                            if (seq == SM[i].rwnd.start_seq) 
                            {
                                int buff_ind = SM[i].rwnd.wndw[seq];
                                memcpy(SM[i].rb[buff_ind], buffer+19, len);
                                SM[i].rb_valid[buff_ind] = 1;
                                SM[i].rwnd.size--;
                                SM[i].lenr[SM[i].rwnd.wndw[seq]] = len;
                                while (SM[i].rwnd.wndw[SM[i].rwnd.start_seq] >= 0 && SM[i].rb_valid[SM[i].rwnd.wndw[SM[i].rwnd.start_seq]] == 1) SM[i].rwnd.start_seq = (SM[i].rwnd.start_seq+1)%256;
                                if (SM[i].rwnd.size == 0) SM[i].nospace = 1;
                                seq = (SM[i].rwnd.start_seq+15)%256; 
                                char ack[14];
                                ack[0] = '0';
                                for (int j = 0; j < 8; j++) ack[8 - j] = ((seq >> j) & 1) + '0';
                                for (int j = 0; j < 4; j++) ack[12 - j] = ((SM[i].rwnd.size >> j) & 1) + '0';
                                ack[13] = '\0';
                                // printf("-------Send2 : %s-------\n",ack);   
                                int k = sendto(SM[i].sid, ack, 14, 0, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
                                if(k<0) perror("Error Sending 2\n");
                            }
                            else 
                            {
                                if (SM[i].rwnd.wndw[seq] >= 0 && SM[i].rb_valid[SM[i].rwnd.wndw[seq]] == 0) 
                                {
                                    int buff_ind = SM[i].rwnd.wndw[seq];
                                    memcpy(SM[i].rb[buff_ind], buffer+19, len);
                                    SM[i].rb_valid[buff_ind] = 1;
                                    SM[i].rwnd.size--;
                                    SM[i].lenr[SM[i].rwnd.wndw[seq]] = len;
                                }
                            }
                        }
                    }
                }
            }
            // pthread_mutex_unlock(&mutex_SM);
            V(sem_SM);
        }
    }
    
}

// ------- SEND THREAD FUNCTION ------- // 
void *S()
{
    while (1) 
    {
        // ------- SLEEP FOR HALF TIME PERIOD ------- // 
        sleep(T / 2);
        // pthread_mutex_lock(&mutex_SM);
        // printf("-------S Thread-------\n");

        // ------- SEND ALL PACKETS ------- //
        P(sem_SM);
        for (int i = 0; i < N; i++) 
        {
            if (SM[i].is_free == 0) 
            {
                struct sockaddr_in serv_addr;
                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons(SM[i].port);
                inet_aton(SM[i].ip_address, &serv_addr.sin_addr);
                int timeout = 0;
                int j = SM[i].swnd.start_seq;

                while (j != (SM[i].swnd.start_seq + SM[i].swnd.size) % 256) 
                {
                    if (SM[i].lastSendTime[j] != -1 && time(NULL) - SM[i].lastSendTime[j] > T) 
                    {
                        timeout = 1;
                        break;
                    }
                    j = (j + 1) % 256;
                }
                if (timeout) 
                {
                    j = SM[i].swnd.start_seq;
                    int start = SM[i].swnd.start_seq;
                    while (j != (start + SM[i].swnd.size) % 256) 
                    {
                        if (SM[i].swnd.wndw[j] != -1) 
                        {
                            char buffer[532];
                            buffer[0] = '1';
                            for (int k = 0; k < 8; k++) buffer[8 - k] = ((j >> k) & 1) + '0';
                            int len = SM[i].lens[SM[i].swnd.wndw[j]];
                            for (int k = 0; k < 10; k++) buffer[18 - k] = ((len >> k) & 1) + '0';
                            memcpy(buffer+19,SM[i].sb[SM[i].swnd.wndw[j]],len);
                            // printf("-------Send3 : %s-------\n",buffer);   
                            
                            int k = sendto(SM[i].sid, buffer, 19+len, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                            Transmitions++;
                            if(k<0) perror("Error Sending 3\n");
                            SM[i].lastSendTime[j] = time(NULL);
                        }
                        j = (j + 1) % 256;
                    }
                }
                else
                {
                    j=SM[i].swnd.start_seq;
                    int start=SM[i].swnd.start_seq;
                    while(j!=(start+SM[i].swnd.size)%256)
                    {
                        if(SM[i].swnd.wndw[j]!=-1 && SM[i].lastSendTime[j]==-1)
                        {
                            char buffer[532];
                            buffer[0] = '1';
                            for (int k = 0; k < 8; k++) buffer[8 - k] = ((j >> k) & 1) + '0';
                            int len = SM[i].lens[SM[i].swnd.wndw[j]];
                            for (int k = 0; k < 10; k++) buffer[18 - k] = ((len >> k) & 1) + '0';
                            memcpy(buffer+19,SM[i].sb[SM[i].swnd.wndw[j]],len);
                            // printf("-------Send4 : %s-------\n",buffer);   

                            int k = sendto(SM[i].sid, buffer, 19+len, 0, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
                            Transmitions++;
                            if(k<0) perror("Error Sending 4\n");
                            SM[i].lastSendTime[j]=time(NULL);
                        }
                        j=(j+1)%256;
                    }
                }
            }
        }
        // pthread_mutex_unlock(&mutex_SM);
        V(sem_SM);
    }
}

// ------- GARBAGE THREAD FUNCTION ------- // 
void * G() 
{
    while (1)
    {
        sleep(T);
        // pthread_mutex_lock(&mutex_SM);
        P(sem_SM);
        for (int i = 0; i < N; i++) 
        {
            if (SM[i].is_free) continue;
            if (kill(SM[i].pid, 0) == 0) continue;
            SM[i].is_free = 1;
        }
        V(sem_SM);
        // pthread_mutex_unlock(&mutex_SM);
    }

}

// ------- MAIN FUNCTION ------- // 
int main() 
{
    signal(SIGINT, sigHandler);

    // ------- CREATION AND INITIALISATION OF SM & SEMOPHERS ------- // 
    pop.sem_num = 0;
    vop.sem_num = 0;
	pop.sem_flg = 0;
    vop.sem_flg = 0;
	pop.sem_op = -1;
    vop.sem_op  = 1;
    shmid = shmget(ftok("/",'P'),N*sizeof(struct SM_entry),0777|IPC_CREAT|IPC_EXCL);
    dummysem1 = semget(ftok("/",'Q'), 1, 0777|IPC_CREAT);
    dummysem2 = semget(ftok("/",'R'), 1, 0777|IPC_CREAT);
    sem_SM = semget(ftok("/",'S'), 1, 0777|IPC_CREAT);
    SM = (struct SM_entry*)shmat(shmid,0,0);

    if(shmid ==-1 || dummysem1 == -1 || dummysem2 == -1 ||sem_SM == -1 || SM == NULL )
    {
        perror("-------error-------\n-------Shared Memory : ");
        exit(EXIT_FAILURE);
    }
    semctl(dummysem1, 0, SETVAL, 0);
    semctl(dummysem2, 0, SETVAL, 0);
    semctl(sem_SM, 0, SETVAL, 1);

    // printf("-------Started SuccessFully-------\n");
    
    // ------- INITIALISATIONS ------- // 
    for(int i=0;i<N;i++) SM[i].is_free = 1;
    Transmitions =0;

    // ------- THREAD CREATIONS ------- // 
    pthread_t recv_thread, send_thread,garb_thread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&recv_thread, &attr, R, NULL);
    pthread_create(&send_thread, &attr, S, NULL);
    pthread_create(&garb_thread, &attr, G, NULL);


    // ------- LOOP FOR SOCKETS AND BIND CREATION ------- // 
    while(1)
    {
        P(dummysem1);
        P(sem_SM);
        for (int i = 0; i < N; i++) 
        {
            if (SM[i].is_free == 0) 
            {
                if(SM[i].do_bind)
                {
                    // ------- BIND CREATION ------- // 
                    struct sockaddr_in serv_addr;
                    serv_addr.sin_family = AF_INET;
                    serv_addr.sin_port = SM[i].self_port;
                    inet_aton(SM[i].self_ip_address, &serv_addr.sin_addr);
                    if(bind(SM[i].sid, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
                    {
                        SM[i].sid=-1;
                        SM[i].err_no=errno;
                    }
                    else 
                    {
                        // printf("-------Bind SuccessFul-------\n");
                    }
                    SM[i].do_bind=0;
                }
                else if(SM[i].do_crt) 
                {
                    // ------- SOCKET CREATION ------- // 
                    int sock_id=socket(AF_INET, SOCK_DGRAM, 0);
                    if(sock_id==-1)
                    {
                        SM[i].sid=-1;
                        SM[i].err_no=errno;
                    }
                    else 
                    {
                        SM[i].sid=sock_id;
                        // printf("-------Sock Created SuccessFull-------\n");
                    }
                    SM[i].do_crt=0;
                }
            }
        }    
        V(dummysem2);
        V(sem_SM);
    }

    pthread_join(recv_thread, NULL);
    pthread_join(send_thread, NULL);
    pthread_join(garb_thread, NULL);

    return 0;
}
