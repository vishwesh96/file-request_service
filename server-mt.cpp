/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <sys/types.h> 
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <cstdlib>
#include <pthread.h>
#include <unistd.h>
#include <queue>
#include <string>
#include <iostream>

using namespace std;


void error(char *msg)
{
    perror(msg);
    exit(1);
}

struct request{
	string filename;
	int client_sockfd;
};

queue<struct request>file_request_queue;
int max_queue_size;

pthread_cond_t server_sleep;
pthread_cond_t worker_sleep;
pthread_mutex_t lock;

void *downloader(void* ){
	while(1){
		
		// acquire lock
		pthread_mutex_lock(&lock);

		while(file_request_queue.empty()){
			// cout<<"Request queue empty...Waiting"<<endl;
			pthread_cond_wait(&worker_sleep, &lock);
			// cout<<"woke up --thread"<<endl;
		}

		struct request r = file_request_queue.front();
		

		if(file_request_queue.size() == max_queue_size){
			file_request_queue.pop();
			// cout<<"space created in request queue...Wake up server"<<endl;
			pthread_cond_signal(&server_sleep);
		}
		else{
			file_request_queue.pop();
		}


		pthread_mutex_unlock(&lock);	//release lock
		// lock released

		/*open the file to read*/
		int fd = open(r.filename.c_str(),O_RDONLY);
		if (fd < 0) {
			error("ERROR opening the file");
			close(r.client_sockfd);
			exit(1);
		}
		 // printf("File opened to read\n");
		char read_buffer[1024];
		 
		 /*read the file and send it to the client */
		while(1){
		 	int n = read(fd,read_buffer,1023);
		 	if(n < 0) {
		 		error("ERROR reading from file"); 	
		 		close(r.client_sockfd);
		 		exit(1);     		
		 	}
		 	// printf("Read %d Bytes from file\n",n);
		 	if(n == 0){
		 		// printf("End of File readeached\n");
		 		close(fd);
		 		break;
		 	}
		 	n = write(r.client_sockfd,read_buffer,1023);
		 	if (n < 0) {
		 		error("ERROR writing to socket");
		 		close(r.client_sockfd);
		 		close(fd);
		 		exit(1);
		 	}
		}

		close(r.client_sockfd);		//close client_sockfd in thread
	}
}



int main(int argc, char *argv[])
{
     int sockfd, client_sockfd, portno, clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     if (argc != 4) {								 //check if user aruguments are correct
         fprintf(stderr,"Error!! Please provide portno number_of_worker_threads server_queue_length \n");
         exit(1);
     }

     /* create socket */

     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0){ 
        error("ERROR opening socket");
    	exit(1);
    }
     /* fill in port number to listen on. IP address can be anything (INADDR_ANY) */

     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);

     /* bind socket to this port number on this machine */

     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0){
     	error("ERROR on binding");
     	exit(1);	
     }
              
     
     /* listen for incoming connection requests */

    listen(sockfd,30);
    clilen = sizeof(cli_addr);
    int worker_threads_count = atoi(argv[2]);
    max_queue_size = atoi(argv[3]);

    vector<pthread_t>worker_threads;
    worker_threads.resize(worker_threads_count);

    for(int i=0 ; i<worker_threads_count ; i++){
    	pthread_create(&worker_threads[i],NULL,downloader,NULL);
    }
    
    // Initialise
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        printf("\n mutex init failed\n");
        return 1;
    }

	if(pthread_cond_init(&server_sleep, NULL) != 0){
        printf("\n conditional variable init failed\n");
        return 1;
	}

	if(pthread_cond_init(&worker_sleep, NULL) != 0){
        printf("\n conditional variable init failed\n");
        return 1;
	}

	/*serve clients continuously*/
     while(1){

	     /* accept a new request, create a new sockfd */
     	pthread_mutex_lock(&lock);
     	
     	while(file_request_queue.size() > max_queue_size && max_queue_size!=0){
     		// cout<<"No space in queue...gonna sleep --server"<<endl;
     		pthread_cond_wait(&server_sleep, &lock);
     		// cout<<"signal received from worker (space created)"<<endl;
     	}

     	pthread_mutex_unlock(&lock);

     	// Request Connection
	    client_sockfd = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen);
	    if (client_sockfd < 0){
	    	error("ERROR on accept,  Client Couldn't be served");
	    	continue;
	    }

	    // Read the request message
	    bzero(buffer,256);
	    n = read(client_sockfd,buffer,255);
	    if (n < 0) {
	   		error("ERROR reading from socket");
	     	close(client_sockfd);
	     	exit(1);
	    } 

	    char filename[256];
	    memcpy(filename,&buffer[4],strlen(buffer)-4);
	    filename[strlen(buffer)-4]='\0';

	    struct request r;
	    r.filename = (string)filename;
	    r.client_sockfd = client_sockfd;

	    // Push file into the request queue and wake up worker thread if necessary
	    //acquire lock
	    pthread_mutex_lock(&lock);

	    if(file_request_queue.empty()){
	    	file_request_queue.push(r);
	    	//signal
	    	// cout<<"signal to workers...request arrived"<<endl;
	    	pthread_cond_broadcast(&worker_sleep);
	    }

	    else{
	    	file_request_queue.push(r);
	    }
	    //release lock
		pthread_mutex_unlock(&lock);


     }
     return 0; 
}
