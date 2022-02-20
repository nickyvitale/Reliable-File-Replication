#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#define HEADERSIZE 4
#define MAXLINE 32768

int isNumeric(char* num){
	for (int i = 0; i < num[i] != '\0'; i++){
                if (!isdigit(num[i])){
                        return 0;
                }
        }
	return 1;
}

int isValidPort(char *port){
	if (isNumeric(port)){
        	if (atoi(port) >= 1024 && atoi(port) <= 65535){
        	        return 1;
        	}
	}
	return 0;
}

int isValidDroppc(char *droppc){
	if (isNumeric(droppc)){
		if (atoi(droppc) >= 0 && atoi(droppc) <= 100){
	                return 1;
        	}	
	}
	return 0;
}

void handleClient(int sockfd, char *outfileName, struct sockaddr *cliaddr, int len, int droppc){
	char *ack = "ACK\n";
	sendto(sockfd, ack, strlen(ack), MSG_CONFIRM, (const struct sockaddr*) cliaddr, len);

	int outfile = open(outfileName, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (outfile < 0){
		fprintf(stderr, "Couldn't open outfile\n");
		exit(1);
	}

	srand(time(NULL));
	int n;
	char rcvdPacket[MAXLINE];
	char payload[MAXLINE];
	int SN;

	int expectedSN = 0;
	while (1){
		n = recvfrom(sockfd, rcvdPacket, MAXLINE, 0, (struct sockaddr *) cliaddr, &len);	

		if (n == 0){ // Final packet sent is always of size 0
			break;
		}

		time_t now;
		struct timeval tv;
		time(&now);
		struct tm *timeStruct = localtime(&now);
		char nowString[MAXLINE], nowStringMS[MAXLINE];
		gettimeofday(&tv, NULL);
		strftime(nowString, sizeof nowString - 1, "%FT%T", timeStruct);
		sprintf(nowStringMS,"%s.%dZ", nowString, tv.tv_usec/1000);

		int noDrop = ((rand() % 99) + 1) > droppc; // drop for the data package
		memcpy(&SN, &rcvdPacket, HEADERSIZE);
		memcpy(&payload, &rcvdPacket[HEADERSIZE], n-HEADERSIZE);

		if (SN == expectedSN && noDrop){ // If packet that is expected next, and not drop the data package
			noDrop = ((rand() % 99) + 1) > droppc; // drop for the ack
			if (noDrop){
				expectedSN++;
				sendto(sockfd, ack, sizeof(ack), MSG_CONFIRM, (const struct sockaddr*) cliaddr, len);
				int wrote = write(outfile, payload, n-HEADERSIZE);
				if (wrote < 0){
					fprintf(stderr, "Failed to write to outfile\n");
					exit(1);
				}
			}
			else{
				fprintf(stderr, "%s, ACK, %d\n", nowStringMS, SN);
			}
		}
		else{ // If not expected packet (because order is wrong, or because packets got dropped)
			if (SN != expectedSN){ // If natural drop
				fprintf(stderr, "%s, DATADROP, %d\n", nowStringMS, SN);
			}
			else{ // If intentional drop
				fprintf(stderr, "%s, DATA, %d\n", nowStringMS, SN);
			}
		}
	}
	close(outfile);
}

int main(int argc, char *argv[]) {
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;
 
	// Some input validation      
	if (argc != 4){
		fprintf(stderr, "EXPECTED: ./myserver port droppc root_folder_path\n");
		exit(1);
	}	
	if (!isValidPort(argv[1])){
		fprintf(stderr, "Not valid port\n");
		exit(1);
	}
	if (!isValidDroppc(argv[2]))
	{
		fprintf(stderr, "Droppc must be number between 0 and 100\n");
		exit(1);
	}

	// Start main code
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
		len = sizeof(cliaddr);	
		char buffer[MAXLINE]; 
 		n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *) &cliaddr, &len);
		buffer[n] = '\0';
		// Make the directories specified by the combined paths of root_folder_path(argv[3]) and outfile_path(from client)
		char *rootFolderPath = strdup(argv[3]);
		char *token = strtok(rootFolderPath, "/");
		char outfile[MAXLINE];
		sprintf(outfile, "%s/", token);
		mkdir(outfile, 0777);
		token = strtok(NULL, "/");
		while(token != NULL ) {
			sprintf(outfile,"%s%s/", outfile, token);
			mkdir(outfile, 0777);
			token = strtok(NULL, "/");
		}
		token = strtok(buffer, "/");
		while (token != NULL){
			sprintf(outfile,"%s%s/", outfile, token);
                	token = strtok(NULL, "/");
 			if (token != NULL){
				mkdir(outfile, 0777);
			}
			else{
				outfile[strlen(outfile) - 1] = '\0';
			}
		}

		handleClient(sockfd, outfile, (struct sockaddr *) &cliaddr, len, atoi(argv[2]));
		free(rootFolderPath);
	}
		
	close(sockfd);
       
	return 0;
}
