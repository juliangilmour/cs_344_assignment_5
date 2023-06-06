#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/wait.h>

// define NUM_PORTS 5
#define SERVER_CODE "enc\0"
#define CODE_LEN 5
#define BUFFER_LEN 1024

// int find_port(int* ports);
void handshake(int client_socket_FD);
char* get_file(int client_socket_FD);
char* encode(char* text, char* key);

int main(int argc, char *argv[]) {
	// Check usage & args
	if (argc < 2) { 
		fprintf(stderr,"USAGE: %s port\n", argv[0]);
		fprintf(stderr,"For example: %s 8080\n", argv[0]); 
		exit(1); 
	}

	int portNumber, listen_socket_FD, client_socket_FD, child_status, spawn_pid;
	struct hostent* hostInfo;
	struct sockaddr_in address;
	socklen_t addrlen = sizeof(address);
	// int ports_in_use = 0;
	int port_offset = 0;
	// int ports[5] = {0};


	// Clear out the address struct
	memset((char*) &address, '\0', sizeof(address));

	// The address should be network capable
	address.sin_family = AF_INET;

	// Store the port number
	portNumber = atoi(argv[1]);// + port_offset;
	port_offset++;
	address.sin_port = htons(portNumber);

	// Get the DNS entry for this host name
	hostInfo = gethostbyname("LOCALHOST"); 
	if (hostInfo == NULL) { 
		fprintf(stderr, "SERVER: ERROR, no such host\n"); 
		exit(1);
	}

	// Copy the first IP address from the DNS entry to sin_addr.s_addr
	memcpy(&address.sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);

	//create socket at address of LOCALHOST
	if((listen_socket_FD = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "SERVER: failed to create socket\n");
		exit(1);
	}
	if(bind(listen_socket_FD, (struct sockaddr*)&address, addrlen) < 0){
		fprintf(stderr, "SERVER: failed to bind socket, port may be in use\n");
		fprintf(stderr, "SERVER: %s\n", strerror(errno));
		exit(1);
	}
	listen(listen_socket_FD, 0);

	while(1){


		client_socket_FD = accept(listen_socket_FD, (struct sockaddr*)&address, &addrlen);
		if(client_socket_FD == -1){
			fprintf(stderr, "SERVER: failed to connect to client\n");
			fprintf(stderr, "%s\n", strerror(errno));
			continue;
		}

		//fork off for subprocess to handle the connection
		spawn_pid = fork();
		if(spawn_pid == -1){
			fprintf(stderr, "SERVER: failed to fork server\n");
			exit(1);
		}
		//subprocess here (doesn't handle concurrency well)
		else if(spawn_pid == 0){
			//subprocess doesnt need listen_socket
			close(listen_socket_FD);

			//stops port from being help by unix maybe
			int yes = 1;
			setsockopt(client_socket_FD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

			//confirm connection
			handshake(client_socket_FD);

			//get file
			char* text = get_file(client_socket_FD);
			char* key = get_file(client_socket_FD);

			//encode
			char* cipher_text = encode(text, key);

			//send to client
			send(client_socket_FD, cipher_text, strlen(cipher_text), 0);

			//exit cleanly hopefully
			close(client_socket_FD);
			free(text);
			free(key);
			free(cipher_text);
			exit(0);
		}
		// close(listen_socket_FD);
		close(client_socket_FD);
		waitpid(-1, &child_status, WNOHANG);
	}
}


//gets and compares server code with client
void handshake(int client_socket_FD){
	char buffer[BUFFER_LEN] = {0};
	while(1){
		recv(client_socket_FD, buffer, BUFFER_LEN, 0);
		if(strlen(buffer) < BUFFER_LEN - 1 && strlen(buffer) != 0){
			break;
		}
	}
	send(client_socket_FD, SERVER_CODE, CODE_LEN, 0);
	if(strcmp(buffer, SERVER_CODE) != 0){
		close(client_socket_FD);
		exit(2);
	}
	return;
}


//recieves packets and loads into dynamic sized string then returns that string
char* get_file(int client_socket_FD){
	int packets_recved = 0;
	char buffer[BUFFER_LEN] = {0};
	char* payload = calloc(0, sizeof(char));
	while(1){
		memset(buffer, 0, BUFFER_LEN);
		recv(client_socket_FD, buffer, BUFFER_LEN, 0);
		packets_recved++;
		payload = realloc(payload, packets_recved * BUFFER_LEN);
		memset(payload + ((packets_recved - 1) * BUFFER_LEN), 0, BUFFER_LEN);
		payload = strcat(payload, buffer);
		if(strlen(buffer) < BUFFER_LEN - 1 && strlen(buffer) != 0){
			break;
		}
	}
	return payload;
}

// //sends packets to client
// void send_file(int client_socket_FD, FILE* file){
// 	char buffer[BUFFER_LEN] = {0};
// 	while(!feof(file)){
// 		memset(buffer, 0, BUFFER_LEN);
// 		fread(buffer, sizeof(char), BUFFER_LEN - 1, file);
// 		send(client_socket_FD, buffer, BUFFER_LEN, 0);
// 	}
// 	return;
// }

//encodes chars
char* encode(char* text, char* key){
	char* ciphertext = calloc(strlen(text), sizeof(char));
	memset(&text[strlen(text) - 1], 0, 1);
	memset(&text[strlen(key) - 1], 0, 1);
	int x, y;
	for(int i = 0; i < strlen(text); i++){

		// ' ' is represented by zero on 0-26 scale
		if(text[i] == ' '){
			x = 0;
		}
		//all other characters are set to 1-26
		else{
			x = (int)text[i] - 64;
		}

		//same for the key
		if(key[i] == ' '){
			y = 0;
		}
		else{
			y = (int)key[i] - 64;
		}

		//combine and mod
		x = (x + y) % 27;

		//if zero, its a space
		if(x == 0){
			x = 32;
		}
		else{
			x += 64;
		}
		//add this character to the cipher
		ciphertext[i] = (char)x;

	}
	return ciphertext;
}