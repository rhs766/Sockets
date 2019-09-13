#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

void error(char *msg)
{
    perror(msg);
    exit(1);
}

// generates encoded message using Walsh codes and
// preps buffer with encoded message and corresponding code to send to child processes
void encode(char buffer[][256], int b[][3], int w[][4])
{
	int enc[4][12];	// holds encoded messages
	int encpos; // iterates through enc
	char buf[256];	// holds what is read from each child
	int bufpos;	// iterates through buf
	int pnum; // process #
	int val; // value sent
	
	// preps and populates buf
	bzero(buf,256);
	bufpos = 0;
	for(int i = 1; i < 4; i++)
	{
		for(int j = 0; j < 3; j++)
		{
			buf[bufpos] = buffer[i][j];
			bufpos++;
		}
	}

	// generates each child's encoded message
	for(int i = 0; i < 9; i+=3)
	{
		encpos = 0;
		pnum = buf[i] - '0';
		val = buf[i+2] - '0';
		for(int j = 0; j < 3; j++)
		{
			for(int k = 0; k < 4; k++)
			{
				enc[pnum][encpos] = b[val][j] * w[pnum][k];
				encpos++;
			}
		}
	}
	
	// generates final encoded message
	for(int i = 0; i < 12; i++)
	{
		enc[0][i] = enc[1][i] + enc[2][i] + enc[3][i];
	}
	
	// prep buffers to send to child processes
	for(int i = 1; i < 4; i++)
	{
		bzero(buffer[i],256);
	}

	// populates buffers with encoded message
	for(int i = 1; i < 4; i++)
	{
		for(int j = 0; j < 12; j++)
		{
			if(enc[0][j] < 0)
			{
				// handles negative numbers for int-char conversion
				buffer[i][j] = (enc[0][j] * -2) + '0';
			}
			else
			{
				buffer[i][j] = enc[0][j] + '0';
			}
		}
	}

	// appends buffers with corresponding code
	for(int i = 1; i < 4; i++)
	{
		for(int k = 1; k < 9; k+=3)
		{
			if((buf[k] - '0') == i)
			{
				for(int m = 0; m < 4; m++)
				{
					if(w[buf[k-1]-'0'][m] < 0)
					{
						// handles negative numbers for int-char conversion
						buffer[i][m+12] = (w[buf[k-1]-'0'][m] * -2) + '0';
					}
					else
					{
						buffer[i][m+12] = w[buf[k-1]-'0'][m] + '0';
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[])
{
	// possible sent numbers in binary
	int b[8][3] = {
		{-1, -1, -1},
		{-1, -1, 1},
		{-1, 1, -1},
		{-1, 1, 1},
		{1, -1, -1},
		{1, -1, 1},
		{1, 1, -1},
		{1, 1, 1}
	};
	
	// Walsh codes
	int w[4][4] = {
		{-1, -1, -1, -1},
		{-1, 1, -1, 1},
		{-1, -1, 1, 1},
		{-1, 1, 1, -1}
	};

	int sockfd, portno, clilen;	
	int newsockfd[3];	// holds newsockfd's for each child
	char buffer[4][256];	// holds what is to be sent or received by each child
    struct sockaddr_in serv_addr, cli_addr;
    int n;

    if (argc < 2) 
	{
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
	{
    	error("ERROR opening socket");
	}
    bzero((char *) &serv_addr, sizeof(serv_addr));
    portno = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
    {
		error("ERROR on binding");
	}
	listen(sockfd,5);
	clilen = sizeof(cli_addr);

	// accept all 3 connections and read from each child into respective buffers
	for(int i = 1; i < 4; i++)
	{
		bzero(buffer[i],256);
		newsockfd[i] = accept(sockfd, (struct sockaddr *) &cli_addr, (socklen_t *)&clilen);
		if (newsockfd[i] < 0) 
		{
			error("ERROR on accept");
		}
		n = read(newsockfd[i],buffer[i],255);
		if (n < 0) 
		{
			error("ERROR reading from socket");
		}

		// print statement to match output format
		printf("Here is the message from child %c: Value = %c, Destination = %c\n",buffer[i][0], buffer[i][2], buffer[i][1]);
	}

	// encodes and preps buffers to write to child processes
	encode(buffer, b, w);

	// sends buffers containing encoded message and 
	// corresponding code to respective child processes
	for(int i = 1; i < 4; i++)
	{
		n = write(newsockfd[i],buffer[i],strlen(buffer[i]));
		if (n < 0) 
		{
			error("ERROR writing to socket");
		}

		// preserves order of print statements
		sleep(1);
	}

	// close sockets
	close(newsockfd[2]);
	close(newsockfd[1]);
	close(newsockfd[0]);
	close(sockfd);
    return 0; 
}
