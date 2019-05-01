/*
TCP sockets - Client
Interprocess communication through sockets by
encoding/decoding messages between server and client

Program reads an input file from stdin using redirection
Example input file (without comments, first value = id of process, second value = 0-7):
3 4 // Process 1 sending to process 3 the integer value 4
1 5 // Process 2 sending to process 1 the integer value 5
2 7 // Process 3 sending to process 2 the integer value 7

Client reads inputs, creates a child process per request, 
	and each child sends their message to the server through their socket.
Server receives each message, encodes them using Walsh codes, 
	and sends these back to their respective clients.
Client receives encoded message with Walsh code and decodes the message.

Example output for Server:
Here is the message from child 1: Value = 4, Destination = 3
Here is the message from child 2: Value = 5, Destination = 1
Here is the message from child 3: Value = 7, Destination = 2

Example output for Client:
Child 1, sending value: 4 to child process 3
Child 2, sending value: 5 to child process 1
Child 3, sending value: 7 to child process 2

Child 1
Signal:-3 1 1 1 1 1 1 -3 -1 -1 3 -1
Code: -1 -1 1 1
Received value = 5

Child 2
Signal:-3 1 1 1 1 1 1 -3 -1 -1 3 -1
Code: -1 1 1 -1
Received value = 7

Child 3
Signal:-3 1 1 1 1 1 1 -3 -1 -1 3 -1
Code: -1 1 -1 1
Received value = 4


To simulate server-client interaction on one machine, use two terminals (one for server, one for client)
First
Compile Server: g++ -std=c++11 -o SocketServer SocketServer.cpp
Run Server: ./SocketServer 51717
*note 51717 is an example port number

Then
Compile Client: g++ -std=c++11 -o SocketClient SocketClient.cpp
Run Client: ./SocketClient localhost 51717 < input.txt
*note localhost is an example address where the server program is located (to simulate the interaction on one machine)
*/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <sys/wait.h>

#define h_addr h_addr_list[0]

// decode message using received signal and code
void decode(char buffer[], int signal[], int code[], int *recval)
{
	int dm[12];	// holds decoded message
	int b[3];	// holds decoded binary
	int dmpos;	// iterates through dm

	// handles negative numbers for int-char conversion
	for(int i = 0; i < 12; i++)
	{
		if((buffer[i] - '0') % 2 == 0)
		{
			signal[i] = (buffer[i] - '0') / (-2);
		}
		else
		{
			signal[i] = buffer[i] - '0';
		}
	}
	for(int i = 12; i < 16; i++)
	{
		if((buffer[i] - '0') % 2 == 0)
		{
			code[i-12] = (buffer[i] - '0') / (-2);
		}
		else
		{
			code[i-12] = buffer[i] - '0';
		}
	}
	
	// generates the decoded message
	dmpos = 0;
	for(int i = 0; i < 3; i++)
	{
		for(int j = 0; j < 4; j++)
		{
			dm[dmpos] = signal[dmpos] * code[j];
			dmpos++;
		}
	}

	// converts decoded message to binary
	dmpos = 0;
	for(int i = 0; i < 3; i++)
	{
		b[i] = (dm[dmpos] + dm[dmpos+1] + dm[dmpos+2] + dm[dmpos+3]) / 4;
		dmpos+=4;
	}

	// convert binary to final value
	if(b[0] == 1)
	{
		*recval += 4;
	}
	if(b[1] == 1)
	{
		*recval += 2;
	}
	if(b[2] == 1)
	{
		*recval += 1;
	}
}

int main(int argc, char *argv[])
{
	pid_t pid; // used to fork
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[256];	// holds what is to be sent or received by each child
	char input[10];		// holds what is read by the input file
	int signal[12];		// holds the signal
	int code[4];		// holds the code
	int recval = 0;		// holds the received value
	int pnum = 1;		// process #

    if (argc < 3) 
	{
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
	bzero(buffer,256);	
	bzero(input,10);

	// read input file
	for(int i = 0; i < 9; i+=3)
	{
		fgets(buffer,255,stdin);
		input[i] = pnum + '0';
		input[i+1]=buffer[0];
		input[i+2]=buffer[2];
		pnum++;
	}
	bzero(buffer,256);

	// fork 3 times to simulate 3 clients, each creating a socket to
	// individually send requests and receive signal and code
	for(int i = 0; i < 9; i+=3)
	{
		if(pid=fork()==0)
		{
			// prep individual requests
			buffer[0]=input[i];
			buffer[1]=input[i+1];
			buffer[2]=input[i+2];

			sockfd = socket(AF_INET, SOCK_STREAM, 0);
			if (sockfd < 0) 
			{
				perror("ERROR opening socket");
				exit(0);
			}
			server = gethostbyname(argv[1]);
			if (server == NULL) 
			{
				fprintf(stderr,"ERROR, no such host\n");
				exit(0);
			}
			bzero((char *) &serv_addr, sizeof(serv_addr));
			serv_addr.sin_family = AF_INET;
			bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
			serv_addr.sin_port = htons(portno);
			if (connect(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0) 
			{	
				perror("ERROR connecting");
				exit(0);
			}
			// print statements to match output format
			printf("Child %c, sending value: %c to child process %c\n", buffer[0], buffer[2], buffer[1]);
			if(input[i]-'0' == 3)
			{
				printf("\n");
			}

			// sending individual request
			n = write(sockfd,buffer,strlen(buffer));
			if (n < 0) 
			{
				perror("ERROR writing to socket");
				exit(0);
			}

			// prep to receive
			bzero(buffer,256);

			// receive signal and code
			n = read(sockfd,buffer,255);
			if (n < 0) 
			{
				perror("ERROR reading from socket");
				exit(0);
			}

			// decode message using received info
			decode(buffer, signal, code, &recval);
			
			// print statements to match output format
			printf("Child %d\n", input[i]-'0');
			printf("Signal:");
			for(int i = 0; i < 12; i++)
			{
				printf("%d ", signal[i]);
			}
			printf("\n");
			printf("Code: ");
			for(int i = 0; i < 4; i++)
			{
				printf("%d ", code[i]);
			}
			printf("\n");
			printf("Received value = %d\n", recval);
			printf("\n");
			
			// close sockets and exit
			close(sockfd);
			exit(0);
		}
		else
		{
			// ensures order of child processes
			sleep(1);
		}
	}
	// ensure all child processes exit properly
	for(int i = 0; i < 3; i++)
	{
		wait(NULL);
	}
   return 0;
}