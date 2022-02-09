#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#define HEADERSIZE 4
#define MAXLINE 32768

int isValidPort(char *port){
        for (int i = 0; i < port[i] != '\0'; i++){
                if (!isdigit(port[i])){
                        return 0;
                }
        }
        if (atoi(port) >= 1024 && atoi(port) <= 65535){
                return 1;
        }
        return 0;
}

void handleClient(int sockfd, char *outfileName, struct sockaddr *cliaddr, int len){
	char *ack = "ACK\n";
	sendto(sockfd, ack, strlen(ack), MSG_CONFIRM, (const struct sockaddr*) cliaddr, len);

	int outfile = open(outfileName, O_WRONLY | O_CREAT | O_TRUNC);
	if (outfile < 0){
		fprintf(stderr, "Couldn't open outfile\n");
		exit(1);
	}

	int n;
	char rcvdPacket[MAXLINE];
	char payload[MAXLINE];
	int SN;
	int expectedSN = 0;
	while (1){
		n = recvfrom(sockfd, rcvdPacket, MAXLINE, 0, (struct sockaddr *) cliaddr, &len);	

		if (n == 0){ // Final packet sent is always of size 0
			printf("End handle function\n");
			break;
		}

		memcpy(&SN, &rcvdPacket, HEADERSIZE);
		memcpy(&payload, &rcvdPacket[HEADERSIZE], n-HEADERSIZE);

		if (SN == expectedSN){ // If packet that is expected next
			printf("now, ACK, %d\n", SN);	
			expectedSN++;
			sendto(sockfd, ack, sizeof(ack), MSG_CONFIRM, (const struct sockaddr*) cliaddr, len);

			int wrote = write(outfile, payload, n-HEADERSIZE);
			if (wrote < 0){
				fprintf(stderr, "Failed to write to outfile\n");
				exit(1);
			}
		}
		else{ // If not expected packet (because order is wrong, or because packets got dropped)
			printf("now, DATA, %d\n", SN);
		}
	}
	close(outfile);
}

int main(int argc, char *argv[]) {
	int sockfd;
	char buffer[MAXLINE];
	struct sockaddr_in servaddr, cliaddr;
 
	// Some input validation      
	if (argc != 3){
		fprintf(stderr, "EXPECTED: ./myserver port droppc\n");
		exit(1);
	}	

	if (!isValidPort(argv[1])){
		fprintf(stderr, "Not valid port\n");
		exit(1);
	}

	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
		fprintf(stderr, "Failed to create socket\n");
		exit(1);
	}            

	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));

	servaddr.sin_family    = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0 ){
		fprintf(stderr, "Failed to bind\n");
		exit(1);
	}

	int len, n;
  
	while (1){
		printf("Enter server main loop\n"); 
		len = sizeof(cliaddr);
   		n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
		handleClient(sockfd, buffer, (struct sockaddr *) &cliaddr, len);
	}
		
	close(sockfd);
       
	return 0;
}
