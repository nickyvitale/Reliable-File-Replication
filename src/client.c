#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <pthread.h>
#define DEFBUFSIZE 100
#define HEADERSIZE 4
#define MAXMTU 32768

struct packet{
	int size;
	char data[MAXMTU];
	int timesSent;
};

struct args{
	int mtu;
	int winsz;
	char *infile;
	char *outfile;
	char *port;
	char *ip;
};

int isNumber(char *str){
	for (int i = 0; i < str[i] != '\0'; i++){
		if (!isdigit(str[i])){
			return 0;
		}
	}
	return 1;
}

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

int isValidMtu(char *mtu){
        for (int i = 0; i < mtu[i] != '\0'; i++){
                if (!isdigit(mtu[i])){
                        return 0;
                }
        }
        if (atoi(mtu) <= HEADERSIZE || atoi(mtu) > MAXMTU){
                return 0;
        }
        return 1;
}

int validIpPart(char *ip){
        for (int i = 0; i < ip[i] != '\0'; i++){
                if (!isdigit(ip[i])){
                        return 0;
               	}
	}	
	if (atoi(ip) >= 0 && atoi(ip) <= 255){
		return 1;
	}
	return 0;
}

int isValidIp(char *ip){
        char *ipCopy = strdup(ip);
        int count = 0;
        char *token = strtok(ipCopy, ".");

        while (token != NULL){
       		if (!validIpPart(token)){
                        free(ipCopy);
                        return 0;
                }
                count++;
                token = strtok(NULL, ".");
        }
        free(ipCopy);
        if (count != 4){
                return 0;
        }
        return 1;
}

void *threadMain(void *argv);

// Main is essentially acting as a driver for the threads
int main(int argc, char *argv[]){
	// Some input validation
	if (argc != 7){
		fprintf(stderr, "EXPECTED: ./myclient #servers serv.conf mtu winsize infilepath outfilepath\n");
		exit(1);
	}
	if (!isValidMtu(argv[3])){
		fprintf(stderr, "Invalid MTU\n");
		exit(1);
	}
	if (!isNumber(argv[4]) || atoi(argv[4]) <= 0){
		fprintf(stderr, "Invalid window size\n");
		exit(1);
	}
	if (!isNumber(argv[1]) || atoi(argv[1]) <= 0){
		fprintf(stderr, "Invalid #servers\n");
		exit(1);
	}

	// Create the threads that handle each server
	FILE *serverData = fopen(argv[2], "r");
	if (serverData == NULL) {
      		fprintf(stderr, "Couldn't open server config file\n");
     		exit(1);
  	}
	char buffer[MAXMTU];
	int i = 0;
	int numServers = atoi(argv[1]);
	pthread_t thread[numServers];
	while (fgets(buffer,MAXMTU, serverData) != NULL){
		if (buffer[0] == '#'){
			continue;
		}
		if (i >= numServers){
			break; // Specified number of servers being handled already, so break
		}
	
		char *port, *ip;
		ip = strtok(buffer, " ");
		port = strtok(NULL, " ");
		port[strlen(port) - 1] = '\0'; // remove the \n character
		if (ip == NULL || port == NULL || !isValidIp(ip) || !isValidPort(port)){
			fprintf(stderr, "Incorrect server config file format\n");
			exit(1);
		}

		struct args *threadArgs = (struct args*)malloc(sizeof(struct args));
		threadArgs->mtu = atoi(argv[3]);
		threadArgs->infile = argv[5];
		threadArgs->outfile = argv[6];
		threadArgs->winsz = atoi(argv[4]);
		threadArgs->port = strdup(port);
		threadArgs->ip = strdup(ip);

		pthread_create(&thread[i], NULL, threadMain, threadArgs);
		i++;
	}
	if (i < numServers){
		fprintf(stderr, "Number of servers requested larger than number of servers provided in server config file\n");
		exit(1);
	}
	fclose(serverData);	

	int *retStatus;
	int exitStatus = 0;
	for (int j = 0; j < i; j++){
		pthread_join(thread[j], (void **) &retStatus);
		if (*retStatus != 0){ // If a thread exited with error
			exitStatus = 1;
		}
		free(retStatus);
	}

	if (exitStatus == 1){
		fprintf(stderr, "At least one thread exited with error\n");
	}
	return exitStatus;
}	

void *threadMain(void *argv){
	struct args *threadArgs = (struct args*)argv;	
	int *returnVal = (int *)malloc(sizeof(int));

	// Do main client work (stuff implemented in Lab 3)
	int mtu = threadArgs->mtu; // claims mtu from the args
	int winsz = threadArgs->winsz;

	int sockfd;
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        	fprintf(stderr, "Failed to create socket\n");
		*returnVal = 1;
		pthread_exit((void*)returnVal);
        }
	
	struct timeval tv; // timeout stuff
	tv.tv_sec = 60; // 60 second timer
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	struct sockaddr_in servaddr; // make sockaddr struct for server
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(threadArgs->port));
	inet_pton(AF_INET, threadArgs->ip, &servaddr.sin_addr);
	
	// Send the first packet to the server-> the name of the outfile
	sendto(sockfd, (const char *)threadArgs->outfile, strlen(threadArgs->outfile), MSG_CONFIRM, (const struct sockaddr *) &servaddr, 			sizeof(servaddr));

	// Get response from server acknowledging that the packet was received
	// If server not available, will timeout and properly exit
	int n, len;
	char recBuffer[mtu];
	for ( ; ; ) {
		n = recvfrom(sockfd, (char *)recBuffer, mtu, 0, (struct sockaddr*) &servaddr, &len);
		if (n < 0){
			if (errno == EWOULDBLOCK) {
				fprintf(stderr, "Timeout-> failed to connect to the server\n");
				*returnVal = 1;
				pthread_exit((void *)returnVal);

			}
			else {
				fprintf(stderr, "recvfrom error\n");
				*returnVal = 1;
				pthread_exit((void *)returnVal);

			}
		}
		else{
			break;
		}
	}

	// Next step is to send actual contents of the file (begin actual meat/potatoes of the protocol)
	int rd;
	int infile = open(threadArgs->infile, O_RDONLY | O_CREAT);
	char message[mtu-HEADERSIZE];
	
	// Create buffer of packets by reading from file and filling up array
	int bufferSize = DEFBUFSIZE;
	struct packet *packetBuffer = (struct packet*)malloc(bufferSize * sizeof(struct packet));
	if (packetBuffer == NULL){
		fprintf(stderr, "Failed to malloc packet buffer\n");
		*returnVal = 1;
		pthread_exit((void *)returnVal);

	}
	
	int count = 0;
	while ((rd = read(infile, message, mtu-HEADERSIZE)) > 0){
		if (count == bufferSize){ // For reallocating buffer if need be
			struct packet *enlargedBuff = (struct packet*)realloc(packetBuffer, bufferSize * 2 *sizeof(struct packet));
			if (enlargedBuff != NULL){
				packetBuffer = enlargedBuff;
			}
			else{
				fprintf(stderr, "Couldn't reallocate buffer\n");
				*returnVal = 1;
				pthread_exit((void *)returnVal);

			}
			bufferSize *= 2;
		}

		packetBuffer[count].size = rd;
		packetBuffer[count].timesSent = 0;
		memcpy(&packetBuffer[count].data, &count, sizeof(count)); // Sequence Number
		memcpy(&packetBuffer[count].data[HEADERSIZE], &message, rd); // Payload
		count++;
	}
	
	// Send contents of file
	tv.tv_sec = 3; // 3 second timer
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	int sendBase = 0;
	int nextSeqNum = 0;
	while (sendBase < count){
		time_t now;
		struct timeval timVal;
		time(&now);
		struct tm *timeStruct = localtime(&now);
		char nowString[DEFBUFSIZE], nowStringMS[DEFBUFSIZE];
		gettimeofday(&timVal, NULL);
		strftime(nowString, sizeof nowString - 1, "%FT%T", timeStruct);
		sprintf(nowStringMS,"%s.%dZ", nowString, timVal.tv_usec/1000);

		while (nextSeqNum < count && nextSeqNum < sendBase + winsz){ // If can still transmit some packets within window
			if (packetBuffer[nextSeqNum].timesSent != 0){ // Detect packet loss
				fprintf(stderr, "Packet loss detected\n");
				if (packetBuffer[nextSeqNum].timesSent == 5){ // Surpassed limit for retransmission
					fprintf(stderr, "Reached max re-transmission limit\n");
					// Let server know that client is closing
					char *finMsg = "";
       			 	 	sendto(sockfd,finMsg, strlen(finMsg), MSG_CONFIRM, (const struct sockaddr*) &servaddr, sizeof(servaddr));
					// Close
					*returnVal = 1;
					pthread_exit((void *)returnVal);
				}
			}
			printf("%s, DATA, %d, %d, %d, %d\n", nowStringMS, nextSeqNum, sendBase, nextSeqNum + 1, sendBase + winsz);
			packetBuffer[nextSeqNum].timesSent++;
			
			sendto(sockfd, packetBuffer[nextSeqNum].data, packetBuffer[nextSeqNum].size + HEADERSIZE, MSG_CONFIRM,								     (const struct sockaddr *) &servaddr, sizeof(servaddr));
			nextSeqNum++;
		}	

		n = recvfrom(sockfd, (char *)recBuffer, mtu, 0, (struct sockaddr*) &servaddr, &len); // Attempt to receive ack
		if (n < 0){
			if (errno == EWOULDBLOCK) { // If timeout on receiving ack-> retransmit
				nextSeqNum = sendBase; // Restart from the packet that failed to receive acknowledgement
                	}
                	else { // Some sort of recv error
				fprintf(stderr, "recvfrom error\n");
				*returnVal = 1;			
				pthread_exit((void *)returnVal);
			}
                }
		else{ // Ack received
			printf("%s, ACK, %d, %d, %d, %d\n", nowStringMS, sendBase, sendBase, nextSeqNum, sendBase + winsz);
			sendBase++;
		}	
	}

	// Send last packet indicating done
	char *finMsg = "";	
	sendto(sockfd, finMsg, strlen(finMsg), 0, (const struct sockaddr*) &servaddr, sizeof(servaddr));

	close(infile);
	close(sockfd);
	free(packetBuffer);

	// Free args that were passed to thread from main

	free(threadArgs->port);
	free(threadArgs->ip);
	free(argv);

	*returnVal = 0;
	pthread_exit((void *)returnVal);
}
