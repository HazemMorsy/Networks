#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fstream>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <sstream>
#include <time.h>

using namespace std;

#define PORT "8080"  // the port users will be connecting to
#define MAXDATASIZE 1024
#define BACKLOG 10	 // how many pending connections queue will hold
#define OK_response "HTTP/1.1 200 OK\r\n\r\n" //"Content-Type: text/html; charset=UTF-8\r\n\r\n"
#define NOTF_response "HTTP/1.1 404 Not Found/r/n"


vector<string> split_request(string s , char delimeter) {
	vector <string> tokens; 
      
    stringstream check(s); 
    string intermediate; 
    while(getline(check, intermediate, delimeter)) 
    { 
        tokens.push_back(intermediate); 
    } 
	return tokens;
}


void read_file(int new_fd , FILE *fp){
	int n; 
    char sendline[MAXDATASIZE] = {0}; 
	
	fseek(fp , 0 , SEEK_END);
	fseek(fp , 0 , SEEK_SET);
	
    while ((n = fread(sendline, sizeof(char), MAXDATASIZE, fp)) > 0) 
    {
        if (n != MAXDATASIZE && ferror(fp))
        {
            perror("Read File Error");
            exit(1);
        }
        
        if (send(new_fd, sendline, n, 0) == -1)
        {
            perror("Can't send file");
            exit(1);
        }
		fprintf(stdout , "DATA SENT IS %d \n" , n);
        memset(sendline, 0, MAXDATASIZE);
    }
	fclose(fp);
}



void write_file(int new_fd , FILE* fp){
    ssize_t n;
    char buff[MAXDATASIZE] = {0};
    while ((n = recv(new_fd, buff, MAXDATASIZE, 0)) > 0) 
    {
        if (n == -1)
        {
            perror("Receive File Error");
            exit(1);
        }
        
        if (fwrite(buff, sizeof(char), n, fp) != n)
        {
            perror("Write File Error");
            exit(1);
        }
        memset(buff, 0, MAXDATASIZE);
    }

	fclose(fp);

}



void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc , char* argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct stat file_stat;
	struct sigaction sa;
	int yes = 1;
	int rv;
	char s[INET6_ADDRSTRLEN];
	char buf [MAXDATASIZE];
	char header [MAXDATASIZE] = {0};
	string main_path = "/home/hazem/Desktop/HTTP";
	int fd;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP


    printf("Server : %s\n" , argv[0]);
    printf("PORT : %s\n" , argv[1]);


	if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd , SOL_SOCKET, SO_REUSEADDR , &yes , sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	printf("server: waiting for connections...\n");


	while(1) {  // main accept() loop
		sin_size = sizeof their_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
		printf("server: got connection from %s\n", s);

		if (!fork()) { // this is the child process



			close(sockfd); // child doesn't need the listener
			double seconds = 0;
               
            clock_t t1;
			clock_t t2;
			t1 = clock();
			while(seconds < 0.004 ){
		    
              printf("seconds now are %f\n" , seconds);
              memset(buf , 0 , MAXDATASIZE);
			  

              // time out for recieve operation is 1 seconds
			  struct timeval tv;
              tv.tv_sec = 1;
              tv.tv_usec = 0;
              setsockopt(new_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);


              // recieve request from client
			  ssize_t data_rec = recv(new_fd , buf , sizeof(buf) , 0);
			  if(data_rec <=0){
				  printf("NO MORE DATA...\n");
				  break;
			  }


			  printf("Client Request: %s \n" , buf);
			
			  // split request on spaces and save them in vectors 
			  vector<string> bufsplitted = split_request(buf , ' ');

			 // memset(buf , 0 , MAXDATASIZE);

			  // check if it is get request
			  if(strcmp(bufsplitted[0].c_str() , "GET") == 0){
				  FILE *fileread;

				  fileread = fopen(bufsplitted[1].c_str() , "r");
				
				  // check if file exists
				  if(fileread == NULL){ // file doesn't exist
				     send(new_fd , NOTF_response , sizeof(NOTF_response) , 0);
				  } 
				  else{ // file exists
				     send(new_fd , OK_response , sizeof(OK_response) , 0);
				     read_file(new_fd , fileread);
				  }

			  }
              else{ // POST

                
			      FILE *recieved_file;
			      fprintf(stdout , "creatre file at server...\n");
			      // get file name
				  string filepostname = bufsplitted[1];
				  char char_arr[filepostname.length()];
				  strcpy(char_arr , filepostname.c_str());
			    
				  // split filepath on (/)
				  vector<string> splitted_name = split_request(char_arr , '/');
			      string filenameonly = splitted_name[6];
			      string filepath = main_path + "/Server_Folders/" + filenameonly;
			    
				   // read file and send it untill it is done
				   recieved_file = fopen(filepath.c_str() , "w");
	              write_file(new_fd , recieved_file);

			  }

			  t2 = clock() - t1;
			  seconds = ((double)t2) / CLOCKS_PER_SEC; 
			}

			close(new_fd);
			exit(0);
		}
		close(new_fd);  
	}

	return 0;
}