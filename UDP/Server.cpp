// Server side implementation of UDP client-server model
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
#include <string>
#include <vector>
#include <sstream>

using namespace std;

#define PORT 8080
#define MAXLINE 1024
#define MAXDATASIZE 500
#define RESULTSFILE "/home/hazem/Desktop/UDP/results.txt"
#define OKRESPONSE "HTTP/1.1 200 OK"
#define NOTFRESPONSE "HTTP/1.1 404 Not Found"

struct packet
{
	uint16_t cksum;
	uint16_t len;
	uint16_t seqno;
	// Data
	char data[MAXDATASIZE];
};

struct ack_packet
{
	uint16_t cksum;
	uint16_t len;
	uint16_t ackno;
};

vector<string> split_request(string s, char delimeter)
{
	vector<string> tokens;

	stringstream check(s);
	string intermediate;

	while (getline(check, intermediate, delimeter))
	{
		tokens.push_back(intermediate);
	}
	return tokens;
}

// Driver code
int main()
{
	int sockfd;
	int additional_bytes = 6;
	FILE * fresults = fopen(RESULTSFILE , "W");
	char buf[BUFSIZ];
	struct packet packet_data;
	struct ack_packet packet_ack;

	struct packet *packet_data_ptr = &packet_data;
	struct ack_packet *packet_ack_ptr = &packet_ack;

	struct sockaddr_in servaddr, cliaddr;

	// Creating socket file descriptor
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));

	// Filling server information
	servaddr.sin_family = AF_INET; // IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);

	// Bind the socket with the server address
	if (bind(sockfd, (const struct sockaddr *)&servaddr,
			 sizeof(servaddr)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	socklen_t len;
	int n;

	len = sizeof(cliaddr); //len is value/resuslt

	// recieve request from client
	ssize_t data_rec = recvfrom(sockfd, buf, sizeof(buf),
								MSG_WAITALL, (struct sockaddr *)&cliaddr,
								&len);
	if (data_rec <= 0)
	{
		printf("NO MORE DATA...\n");
	}

	printf("Client Request: %s \n", buf);

	// split request on spaces and save them in vectors
	vector<string> bufsplitted = split_request(buf, ' ');

	// memset(buf , 0 , MAXDATASIZE);

	// check if it is get request
	if (strcmp(bufsplitted[0].c_str(), "GET") == 0)
	{
		FILE *fileread;

		printf("file server recieved: %s\n", bufsplitted[1].c_str());

		fileread = fopen(bufsplitted[1].c_str(), "r");

		// check if file exists
		if (fileread == NULL)
		{ // file doesn't exist
			sendto(sockfd, NOTFRESPONSE, sizeof(NOTFRESPONSE),
				   MSG_CONFIRM, (const struct sockaddr *)&cliaddr,
				   len);
		}
		else
		{ // file exists
			sendto(sockfd, OKRESPONSE, sizeof(OKRESPONSE),
				   MSG_CONFIRM, (const struct sockaddr *)&cliaddr,
				   len);

			int windowsize = 1;
			int total_acks = 0;
			int prop = 10;
			int threshold = 8;
			clock_t start_t, end_t;
			clock_t  start_graph = clock();
			double total_t;
			double timeout = 0.0008; // timeout after 1 sec
			start_t = clock();
			int toseek = 0;
			int toseektimeout = 0;
			int seqnotimeout = 0;
			int win_start = 1;
			packet_data_ptr->seqno = 0;
			int last_ackno = 0;
			int packetsentend = 500;
			

            printf("-----------start----------------------------------\n");

            
			while (1)
			{

				memset(packet_data_ptr->data, 0, MAXDATASIZE);
				fseek(fileread, toseek, SEEK_SET);
				int packetsent_size = fread(packet_data_ptr->data, 1, sizeof(packet_data_ptr->data), fileread);
				toseek += packetsent_size;
				toseektimeout += packetsent_size;
				packet_data_ptr->len = sizeof(packet_data_ptr->data);
				packet_data_ptr->seqno += 1;
				seqnotimeout += 1;

				//printf("packet size to be sent: %d\n", packetsent_size);

				//printf("data sent: %s\n", packet_data_ptr->data);

				int totalsize_sent = packetsent_size + additional_bytes;

				//printf("total size sent: %d\n", totalsize_sent);

				int randno = rand() % 100 + 1;


				printf("random no : %d \n" , randno);

				if (randno > prop && packet_data_ptr->seqno <= (win_start + windowsize) )
				{
					packetsentend = packetsent_size;
					printf("packet sent with seq no : %d \n" , packet_data_ptr->seqno);
					printf("packet size to be sent: %d\n", packetsent_size);
				    printf("data sent: %s\n", packet_data_ptr->data);
					printf("total size sent: %d\n", totalsize_sent);

					sendto(sockfd, (packet *)packet_data_ptr, totalsize_sent,
						   MSG_CONFIRM, (const struct sockaddr *)&cliaddr,
						   len);
				}

				int ackpacket_recieved = recvfrom(sockfd, (ack_packet *)packet_ack_ptr, sizeof(packet_ack),
												  MSG_DONTWAIT, (struct sockaddr *)&cliaddr,
												  &len);

				printf("ack packet %d\n" , packet_ack_ptr->ackno);

				if (packet_ack_ptr->ackno == last_ackno)
				{
					end_t = clock();
					total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
					toseektimeout -= packetsent_size;
					seqnotimeout -= 1;

					printf("total time passed %f \n" , total_t);
					printf("total timeout %f \n" , timeout);


					if (total_t >= timeout)
					{
						threshold = windowsize / 2;
						windowsize = 1;
						printf("TIMING : %f**************************************************" , (double)(clock() - start_graph) / CLOCKS_PER_SEC);
						printf("WINDOW : %d\n" , windowsize);

						total_acks = 0;
						toseek = toseektimeout;
						packet_data_ptr->seqno = seqnotimeout;
						printf("seqno timeout : %d\n" , packet_data_ptr->seqno);
						win_start = seqnotimeout + 1;
						start_t = clock();
					}
					else{
						threshold = windowsize / 2;
						if(threshold == 0){
							threshold = 1;
						}
						windowsize = threshold;
						printf("TIMING : %f**************************************************" , (double)(clock() - start_graph) / CLOCKS_PER_SEC);
						printf("WINDOW : %d\n" , windowsize);
					}
				}
				else if (packet_ack_ptr->ackno - last_ackno == 1)
				{
					if(windowsize >= threshold){
						total_acks += 1;
						if(total_acks >= windowsize){
							windowsize += 1;
							total_acks = 0;
							printf("TIMING : %f**************************************************" , (double)(clock() - start_graph) / CLOCKS_PER_SEC);
						    printf("WINDOW : %d\n" , windowsize);
						}
					}
					else{
						//total_acks += 1;
						windowsize += 1;
						printf("TIMING : %f**************************************************" , (double)(clock() - start_graph) / CLOCKS_PER_SEC);
						printf("WINDOW : %d\n" , windowsize);
					}
					
					printf("ack recieved sequentially \n");
					win_start += 1;
					last_ackno += 1;
					start_t = clock();
					if(packetsentend < MAXDATASIZE){

                        /*packet_data_ptr->seqno = 10000;
						sendto(sockfd, (packet *)packet_data_ptr, totalsize_sent,
						   MSG_CONFIRM, (const struct sockaddr *)&cliaddr,
						   len);*/

						printf("END OF SERVER\n");
						break;
					}
				}

				printf("------------------------------------------\n");
			}
		}
	}

	return 0;
}