
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sstream>
#include <string>
#include <vector>

using namespace std;


#define PORT "8080"  // the port users will be connecting to
#define MAXDATASIZE 1024
//#define FILENAME "/home/hazem/Desktop/doc2_rec.html"
#define REQUESTFILENAME "/home/hazem/Desktop/HTTP/requests.txt"
#define OK_response "HTTP/1.1 200 OK\r\n\r\n"// "Content-Type: text/html; charset=UTF-8\r\n\r\n"
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



void read_file(int sockfd , FILE *fp){
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
        
        if (send(sockfd, sendline, n, 0) == -1)
        {
            perror("Can't send file");
            exit(1);
        }
		fprintf(stdout , "DATA SENT IS %d \n" , n);
        memset(sendline, 0, MAXDATASIZE);
    }
}



void write_file(int sockfd , FILE* fp){
    ssize_t n;
    char buff[MAXDATASIZE] = {0};
    while ((n = recv(sockfd, buff, MAXDATASIZE, 0)) > 0) 
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
	int sockfd, numbytes;
	ssize_t len;
	struct addrinfo hints, *servinfo, *p;
	char s[INET6_ADDRSTRLEN];
	int rv;
	char buf [BUFSIZ];
	int file_size;
	int remain_data = 0;
	ssize_t total;
	string main_path = "/home/hazem/Desktop/HTTP";


	if(argc != 2){
		fprintf(stderr , "usage: client hostname\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;


    char* token = strtok(argv[1] , ":");
    char* ip = token;
	char* port;
	while (token!= NULL)
	{
		token = strtok(NULL , ":");
		port = token;
		break;
	}
	
	printf("IP : %s\n" , ip);
	printf("PORT : %s\n" , port);

	if ((rv = getaddrinfo(ip, port, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if(connect(sockfd , p->ai_addr , p->ai_addrlen) == -1){
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}


	if (p == NULL)  {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}


	inet_ntop(p->ai_family , get_in_addr((struct sockaddr *)p->ai_addr) , s , sizeof s);

	printf("client: connecting to %s\n" , s);

	freeaddrinfo(servinfo);

    FILE* fp;
	char str[MAXDATASIZE];

	fp = fopen(REQUESTFILENAME , "r");


    if (fp == NULL){
		fprintf(stdout , "file open error\n");
	}


    // read lines from requests file 

	while (fgets(str , MAXDATASIZE , fp) != NULL)
	{
		fprintf(stdout , "request: %s\n" , str);
		vector<string> splitted = split_request(str , ' ');
	
		// check if it is get request
		if(strcmp(splitted[0].c_str() ,"GET") == 0){
			
			// send get request to server
			send(sockfd , str , sizeof(str) , 0);

			// recieve the response from the system
			recv(sockfd , buf , sizeof(buf) , 0);
			fprintf(stdout , "SERVER RESPONSE : %s \n" , buf);


            
            FILE *recieved_file;
            if(strcmp(buf , OK_response) == 0){
			    
				fprintf(stdout , "creatre file at client...\n");
			    // get file path
				string filepostname = splitted[1];
				char char_arr[filepostname.length()];
				strcpy(char_arr , filepostname.c_str());
			    
				// split path on (/)
				vector<string> splitted_name = split_request(char_arr , '/');
			    string filenameonly = splitted_name[6];

				// revceive file from server
			    string filepath = main_path + "/Client_Folders/" + filenameonly;
			    recieved_file = fopen(filepath.c_str() , "w");
	            write_file(sockfd , recieved_file);
			}
			else{
				fprintf(stdout , "server response %s \n" , buf);
			}

		}
		else{  // POST
		    // send post request
		    send(sockfd , str , sizeof(str) , 0);
            // send message body
			FILE *fileread;
			fileread = fopen(splitted[1].c_str() , "r");
			read_file(sockfd , fileread);
		}

		memset(str , 0 , MAXDATASIZE);
		memset(buf , 0 , MAXDATASIZE);
	}

    printf("close connection\n");
	close(sockfd);
	



	return 0;
}