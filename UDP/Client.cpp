#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <vector>
#include <sstream>

using namespace std;

#define PORT 8080
#define MAXLINE 1024
#define MAXDATASIZE 500
#define REQUESTFILENAME "/home/hazem/Desktop/UDP/requests.txt"
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
    string mainpath = "/home/hazem/Desktop/UDP";
    char buf[BUFSIZ];
    char str_request[MAXDATASIZE];
    FILE *fp = fopen(REQUESTFILENAME, "r");
    struct packet packet_data;
    struct ack_packet packet_ack;
    struct packet *packet_data_ptr = &packet_data;
    struct ack_packet *packet_ack_ptr = &packet_ack;

    //strcpy(packet_data_ptr->data , "Hello from client");
    //printf("okkkk %s\n" , packet_data_ptr->data);

    struct sockaddr_in servaddr;

    // Creating socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Filling server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    socklen_t len;
    int n;

    if (fgets(str_request, sizeof(str_request), fp) != NULL)
    {
        str_request[strlen(str_request) - 1] = '\0';
        printf("request: %s\n", str_request);
        vector<string> splitted = split_request(str_request, ' ');

        // check if it is get request
        if (strcmp(splitted[0].c_str(), "GET") == 0)
        {

            // send get request to server
            //send(sockfd , str , sizeof(str) , 0);
            sendto(sockfd, str_request, sizeof(str_request),
                   MSG_CONFIRM, (const struct sockaddr *)&servaddr,
                   sizeof(servaddr));

            // recieve the response from the system
            recvfrom(sockfd, buf, sizeof(buf),
                     MSG_WAITALL, (struct sockaddr *)&servaddr,
                     &len);
            printf("SERVER RESPONSE : %s \n", buf);

            FILE *writtable_file;

            if (strcmp(buf, OKRESPONSE) == 0)
            {
                printf("creatre file at client to write ...\n");

                string filerootpath = splitted[1];
                char filerootpathchar[filerootpath.length()];
                strcpy(filerootpathchar, filerootpath.c_str());

                vector<string> dirs = split_request(filerootpathchar, '/');
                string filelastname = dirs[6];

                string newpath = mainpath + "/Client_Folder/" + filelastname;
                writtable_file = fopen(newpath.c_str(), "w");

                int ack_waited = 1;
                int toseek = 0;

                printf("----------------------------start----------------------\n");
                
                while (1)
                {

                    memset(packet_data_ptr->data , 0 , MAXDATASIZE);

                    int recieved_size = recvfrom(sockfd, (packet *)packet_data_ptr, sizeof(packet_data),
                                                 MSG_WAITALL, (struct sockaddr *)&servaddr,
                                                 &len);

                    /*if(packet_data_ptr->seqno == 10000){
                        break;
                    }*/


                    if (ack_waited == packet_data_ptr->seqno)
                    {
                        printf("desired packet recieved \n");
                        printf("recieved size : %d\n", recieved_size);
                        printf("data recieved : %s\n", packet_data_ptr->data);
                        printf("seq no of recieved packet is : %d\n", packet_data_ptr->seqno);


                        fseek(writtable_file , toseek , SEEK_SET);
                        fwrite(packet_data_ptr->data, 1, recieved_size - additional_bytes, writtable_file);
                        toseek += (recieved_size - additional_bytes);

                        packet_ack_ptr->ackno = ack_waited;

                        sendto(sockfd, (ack_packet *)packet_ack_ptr, sizeof(packet_ack),
                               MSG_CONFIRM, (const struct sockaddr *)&servaddr,
                               sizeof(servaddr));

                        ack_waited += 1;

                        if(recieved_size - additional_bytes < MAXDATASIZE){
                            break;
                        }

                    }
                    else{
                        printf("not the desired packet recieved \n");
                        printf("recieved size : %d\n", recieved_size);
                        printf("data recieved : %s\n", packet_data_ptr->data);
                        printf("seq no of recieved packet is : %d\n", packet_data_ptr->seqno);

                        packet_ack_ptr->ackno = ack_waited - 1;
                        sendto(sockfd, (ack_packet *)packet_ack_ptr, sizeof(packet_ack),
                               MSG_CONFIRM, (const struct sockaddr *)&servaddr,
                               sizeof(servaddr));
                    }

                    


                    printf("--------------------------------------------------\n");
                }
            }
            else
            {

                // not found file
                printf("server response %s \n", buf);
            }
        }
    }


    close(sockfd);
    return 0;
}