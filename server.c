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



void error(char *msg)
{
    perror(msg);
    exit(1);
}


void *clean_up(){
	// printf("In clean up thread\n");
	while(1){											//when no child process wait() returns. So,
		int status;
	    while(1){
	    	int m=waitpid(-1,&status,WNOHANG);
	    	if(m==0||m==-1)break;
	    	// printf("Cleaned up child process : %d\n",m);	    	
	    }
	sleep(5);
	}
}

int main(int argc, char *argv[])
{
     int sockfd, client_sockfd, portno, clilen;
     char buffer[256];
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
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

     listen(sockfd,1000);
     clilen = sizeof(cli_addr);

     pthread_t clean_up_thread;
	pthread_create(&clean_up_thread, NULL, clean_up, NULL);


 	 // int start=1;
     while(1){

	     /* accept a new request, create a new sockfd */
	     client_sockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
	     if (client_sockfd < 0){
	     	error("ERROR on accept,  Client Couldn't be served");
	     	continue;
	     }


		int pid=fork();
		if(pid < 0){
			error("ERROR forking, Client Couldn't be served");
			exit(1);										//exit(1)s are required so that the child shouldn't start accepting connections and forking
		}
		// if(pid){
		// 	if(start){
		// 		pthread_create(&clean_up_thread, NULL, clean_up, NULL);
		// 		start=0;
		// 	}
		// 	else{
		// 		if(pthread_cancel(clean_up_thread)){
		// 			error("Couldn't cancel clean_up_thread");
		// 		}
		// 		else{
		// 			if(pthread_create(&clean_up_thread, NULL, clean_up, NULL)){
		// 				error("Couldn't create clean_up_thread");
		// 				start = 1;
		// 			}
		// 		}
		// 	}
		// }
		if(!pid){
			 // printf("In child process\n");
			 if(close(sockfd) < 0);    //couldn't close listen socket in child
		     /* read message from client */

		     bzero(buffer,256);
		     n = read(client_sockfd,buffer,255);
		     if (n < 0) {
		     	error("ERROR reading from socket");
		     	close(client_sockfd);
		     	exit(1);
		     } 

		     // printf("Here is the message: %s\n",buffer);

		     char filename[256];
		     memcpy(filename,&buffer[4],strlen(buffer)-4);
		     filename[strlen(buffer)-4]='\0';
		     // printf("Filename : %s Filename Size : %d\n",filename,strlen(filename));
		     int fd = open(filename,O_RDONLY);
		     if (fd < 0) {
		     	error("ERROR opening the file");
		     	close(client_sockfd);
		     	exit(1);
		     }
		     // printf("File opened to read\n");
		     char read_buffer[1024];
		     while(1){
		     	n = read(fd,read_buffer,1023);
		     	if(n < 0) {
		     		error("ERROR reading from file"); 	
		     		close(client_sockfd);
		     		exit(1);     		
		     	}
		     	// printf("Read %d Bytes from file\n",n);
		     	if(n == 0){
		     		// printf("End of File readeached\n");
		     		close(fd);
		     		break;
		     	}
		     	n = write(client_sockfd,read_buffer,1023);
		     	if (n < 0) {
		     		error("ERROR writing to socket");
		     		close(client_sockfd);
		     		exit(1);
		     	}
		     	// printf("Wrote %d Bytes to socket\n",n);
		     }

		    close(client_sockfd);		//close client_sockfd in child
		    // printf("Closed client_socket in child\n");
		 	exit(0); 					//successfully sent file
	 	}
		// printf("Closed client_socket in parent\n");	 	
	 	close(client_sockfd);		//close client_sockfd in parent

     }
     return 0; 
}