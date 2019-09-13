# Sockets
TCP/IP server-client sockets for IPC  

Encodes/Decodes messages between server and client programs  
through interprocess communication with sockets.  

Client reads inputs, creates a child process per request,  
	and each child sends their message to the server through their socket.  
Server receives each message, encodes them using Walsh codes,  
	and sends these back to their respective clients.  
Client receives encoded message with Walsh code and decodes the message.  

Program reads an input file from stdin using redirection.  

Example input file (without comments, first value = id of process, second value = 0-7):  
3 4 // Process 1 sending to process 3 the integer value 4  
1 5 // Process 2 sending to process 1 the integer value 5  
2 7 // Process 3 sending to process 2 the integer value 7  

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

To simulate server-client interaction on one machine, use two terminals (one for server, one for client).  
First:  
Compile Server: g++ -std=c++11 -o SocketServer SocketServer.cpp  
Run Server: ./SocketServer 51717  
*note 51717 is an example port number  

Then:  
Compile Client: g++ -std=c++11 -o SocketClient SocketClient.cpp  
Run Client: ./SocketClient localhost 51717 < input.txt  
*note localhost is an example address where the server program is located (to simulate the interaction on one machine)  
