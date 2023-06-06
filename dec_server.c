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
#define SERVER_CODE "dec\0"
#define CODE_LEN 5
#define BUFFER_LEN 1024

// int find_port(int* ports);
void handshake(int client_socket_FD);
char* get_file(int client_socket_FD);
char* decode(char* text, char* key);

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

	// Print the IP address in the socket address as a dotted decimal string
	fprintf(stderr, "SERVER: IP address for %s: %s\n", argv[1], inet_ntoa(address.sin_addr));

	
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

	//listen for connectio nfrom client
	listen(listen_socket_FD, 0);

	while(1){

		//accept connection
		client_socket_FD = accept(listen_socket_FD, (struct sockaddr*)&address, &addrlen);
		if(client_socket_FD == -1){
			fprintf(stderr, "SERVER: failed to connect to client\n");
			fprintf(stderr, "%s\n", strerror(errno));
			continue;
		}

		//fork subprocess to handle connection
		spawn_pid = fork();
		if(spawn_pid == -1){
			fprintf(stderr, "SERVER: failed to fork server\n");
			exit(1);
		}
		else if(spawn_pid == 0){
			//subprocess doesnt need listen_socket
			close(listen_socket_FD);

			//this was in the instructions. maybe makes ports reuable
			int yes = 1;
			setsockopt(client_socket_FD, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

			//exchange IDs with client
			handshake(client_socket_FD);

			//get files from client
			char* text = get_file(client_socket_FD);
			
			char* key = get_file(client_socket_FD);

			//decode
			char* deciphered_text = decode(text, key);

			//return decoded message
			send(client_socket_FD, deciphered_text, strlen(deciphered_text), 0);
			//exit cleanly
			close(client_socket_FD);
			free(text);
			free(key);
			free(deciphered_text);
			exit(0);
		}
		close(client_socket_FD);
		waitpid(-1, &child_status, WNOHANG);
	}
}

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
		fprintf(stderr, "SERVER: closing connection, client mismatch\n");
		close(client_socket_FD);
		exit(2);
	}
	return;
}

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

// void send_file(int client_socket_FD, FILE* file){
// 	char buffer[BUFFER_LEN] = {0};
// 	while(!feof(file)){
// 		memset(buffer, 0, BUFFER_LEN);
// 		fread(buffer, sizeof(char), BUFFER_LEN - 1, file);
// 		fprintf(stderr, "\nbuffer: %s", buffer);
// 		send(client_socket_FD, buffer, BUFFER_LEN, 0);
// 	}
// 	return;
// }

char* decode(char* text, char* key){
	char* ciphertext = calloc(strlen(text), sizeof(char));
	memset(&text[strlen(text) - 1], 0, 1);
	memset(&text[strlen(key) - 1], 0, 1);
	int x, y;
	for(int i = 0; i < strlen(text); i++){
		if(text[i] == ' '){
			x = 0;
		}
		else{
			x = (int)text[i] - 64;
		}
		if(key[i] == ' '){
			y = 0;
		}
		else{
			y = (int)key[i] - 64;
		}
		if((x = x - y) < 0){
			x = 27 + x;
		}
		x = x % 27;
		if(x == 0){
			x = 32;
		}
		else{
			x += 64;
		}
		ciphertext[i] = (char)x;

	}
	return ciphertext;
}