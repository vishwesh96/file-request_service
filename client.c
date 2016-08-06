#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <stdlib.h>
#include <string.h>

struct arguments{
    int port_no;
    int think_time;
    int duration;
    char mode[7];
    char hostname[16];
    int *thread_num_requests;		//can be global
    double *thread_response_times;
    int thread_id;
    struct timeval start_time;
};
void error(char *msg)
{
    perror(msg);
    exit(0);
}

void *client(void* s){
    
    struct arguments* t =(struct arguments*)s;          //void* copy
    int thread_id = t->thread_id;
    struct timeval start_time = t->start_time;

    int sockfd, n;
    double response_time=0.0;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    int r=0;
    if(!strcmp(t->mode,"fixed")){
        r=rand()%10000;
    }
    /* create socket, get sockfd handle */
    struct timeval current_time;
    while(1){
        gettimeofday(&current_time,NULL);
        //check time duration
        if(current_time.tv_sec > start_time.tv_sec + t->duration){
            // printf("Time up, terminating thread %d\n",thread_id);
            t->thread_response_times[thread_id]=response_time;
            pthread_exit();
        }
        
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0){
            error("ERROR opening socket");
            continue;               //ignoring think_time
        }
        /* fill in server address in sockaddr_in datastructure */

        server = gethostbyname(t->hostname);
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            close(sockfd);
            exit(1);
        }
        bzero((char *) &serv_addr, sizeof(serv_addr));          //clear serv_addr
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, 
             (char *)&serv_addr.sin_addr.s_addr,
             server->h_length);
        serv_addr.sin_port = htons(t->port_no);

        /* connect to server */

        if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) {
            error("ERROR connecting");
            close(sockfd);
            continue;               //ignoring think_time
        }
        // printf("Connected to server in thread %d\n",thread_id);

        /* send user message to server */
        if(!strcmp(t->mode,"random")){
            r=rand()%10000;
        }

        char buffer[256];
        strcpy(buffer,"get files/foo");
        char str[5];
        sprintf(str,"%d",r);
        strcat(buffer,str);
        strcat(buffer,".txt");

        // printf("Writing : %s to socket in thread %d\n",buffer,thread_id);

        n = write(sockfd,buffer,strlen(buffer));
        if (n < 0) {
            error("ERROR writing to socket");
            close(sockfd);
            continue;                   // ignoring think_time
        }
        struct timeval t1,t2;
        gettimeofday(&t1,NULL);
        char read_buffer[1024];
        /* read reply from server */
        // printf("Starting to read file in thread : %d\n",thread_id);
        while(1){
            n = read(sockfd,read_buffer,1023);
            if (n < 0) {
                 error("ERROR reading from socket");
                 break;
            }
            if(n==0){
                // printf("File read completely in thread : %d\n",thread_id);
                gettimeofday(&t2,NULL); 
                response_time+=((t2.tv_sec-t1.tv_sec)*1000.0 + (t2.tv_usec-t1.tv_usec)/1000.0);
                t->thread_num_requests[thread_id]++;
                break;
            }
            // printf("%s\n",read_buffer);   
        }
        close(sockfd);
        // printf("Waiting for think_time in thread : %d\n",thread_id);
        sleep(t->think_time);
    }

}

int main(int argc, char *argv[])
{
    if (argc < 7) {
       fprintf(stderr,"usage %s hostname port number_users duration think_time mode ", argv[0]);
       exit(0);
    }

    int num_threads=atoi(argv[3]);

    struct arguments t;
    t.port_no = atoi(argv[2]);
    strcpy(t.hostname,argv[1]);
    t.duration = atoi(argv[4]);
    t.think_time = atoi(argv[5]);
    strcpy(t.mode, argv[6]);

    // int *thread_num_requests = new int[num_threads];
    // double *thread_response_times = new double[num_threads];
    int *thread_num_requests = malloc(sizeof(int)*num_threads);
    double  *thread_response_times = malloc(sizeof(double)*num_threads);

    t.thread_num_requests = thread_num_requests;
    t.thread_response_times = thread_response_times;

    // pthread_t * client_thread = new pthread_t[num_threads];
    pthread_t *client_thread = malloc(sizeof(pthread_t)*num_threads);
    int i=0;
    for(i=0;i<num_threads;i++){
        t.thread_id=i;
        gettimeofday(&t.start_time,NULL);                       //each thread runs for duration
        if(pthread_create(&client_thread[i],NULL,client,&t)){
            // printf("ERROR in creating thread %d \n",i);
        }
        // printf("Successfully created thread %d \n",i);
    }
    for(i=0;i<num_threads;i++){
        if(pthread_join(client_thread[i],NULL)){
            printf("ERROR in joining thread %d ",i);
        }
        // printf("Successfully joined thread %d \n",i);
    }

    int total_requests=0;
    double total_response_time=0.0;
    for(i=0;i<num_threads;i++){
        total_requests+=thread_num_requests[i];
        total_response_time+=thread_response_times[i];
    }

    printf("Throughuput(req/s) : %f\nAverage Response Time(in ms) : %f \n",((double)total_requests)/t.duration,total_response_time/total_requests);
    free(thread_num_requests);
    free(thread_response_times);
    free(client_thread);
    return 0;
}
