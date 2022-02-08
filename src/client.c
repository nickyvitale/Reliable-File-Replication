#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#define DEFBUFSIZE 10
#define HEADERSIZE 4
#define MAXMTU 32768

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

int main(int argc, char *argv[]){
	// Some input validation
	if (argc != 7){
		fprintf(stderr, "EXPECTED: ./myclient ip port mtu winsize infilepath outfilepath\n");
		exit(1);
	}
	if (!isValidPort(argv[2])){
		fprintf(stderr, "Invalid port\n");
		exit(1);
	}
	if (!isValidIp(argv[1])){
		fprintf(stderr, "Invalid Ip\n");
		exit(1);
	}
	if (!isValidMtu(argv[3])){
		fprintf(stderr, "Invalid MTU\n");
		exit(1);
	}
	

	// Start main code
	int mtu = atoi(argv[3]); // claims mtu from the args
	int sockfd;
	if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
        	fprintf(stderr, "Failed to create socket\n");
		exit(1);
        }
	
	struct timeval tv; // timeout stuff
	tv.tv_sec = 30; // 30 second timer
	tv.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	
	struct sockaddr_in servaddr; // make sockaddr struct for server
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(atoi(argv[2]));
	inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	
	int rd, n, len;
	int infile = open(argv[5], O_RDONLY | O_CREAT);
	char message[mtu-HEADERSIZE], recBuffer[mtu];
	
	// Create buffer of packets by reading from file and filling up array
	int bufferSize = DEFBUFSIZE;

	char **packetBuffer = (char**)malloc(bufferSize * sizeof(char *));

	int count = 0;
	char packet[mtu];
	while ((rd = read(infile, message, mtu-HEADERSIZE)) > 0){
		if (count == bufferSize){
			char **enlargedBuff = (char **)realloc(packetBuffer, bufferSize * 2 *sizeof(char *));
			if (enlargedBuff != NULL){
				packetBuffer = enlargedBuff;
			}
			else{
				fprintf(stderr, "Couldn't reallocate buffer\n");
				exit(1);
			}
			bufferSize *= 2;
		}
		
		memcpy(&packet, &count, sizeof(count));
		memcpy(&packet[HEADERSIZE], &message, rd);
		
		char ctn[rd];
		memcpy(&ctn, &packet[HEADERSIZE], rd);
		printf("%s\n", ctn);
		count++;
	}
	
	// Send the first packet to the server-> the name of the outfile
	sendto(sockfd, (const char *)argv[6], strlen(argv[6]), MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));

	// Get response from server acknowledging that the packet was received
	n = recvfrom(sockfd, (char *)recBuffer, mtu, 0, (struct sockaddr*) &servaddr, &len);

	// Send contents of file
	for (int i = 0; i < count; i++){
		sendto(sockfd, packet, mtu, MSG_CONFIRM, (const struct sockaddr *) &servaddr, sizeof(servaddr));
	}

	// Send last packet indicating done
	char *finMsg = "";
	sendto(sockfd, finMsg, strlen(finMsg), 0, (const struct sockaddr*) &servaddr, sizeof(servaddr));

	close(infile);
	close(sockfd);

	return 0;
}
