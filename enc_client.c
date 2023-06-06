/*
https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
https://www.programiz.com/c-programming/c-file-input-output
https://man7.org/linux/man-pages/man2/accept.2.html
https://en.cppreference.com/w/c/memory/realloc
*/
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

#define SERVER_CODE "enc\0"
#define CODE_LEN 5
#define BUFFER_LEN 1024

void send_file(int server_socket_FD, FILE* file);
int verify_key_size(FILE* text, FILE* key);
int validate_file(FILE* file);
void handshake(int server_socket_FD);
char* get_file(int server_socket_FD);

int main(int argc, char *argv[]) {
	// Check usage & args
	if (argc < 4) {
		exit(1); 
	}

	int portNumber, server_socket_FD;
	struct hostent* hostInfo;
	struct sockaddr_in address;
	int addrlen = sizeof(address);
	
	//open files
	FILE* text = fopen(argv[1], "r");
	FILE* key = fopen(argv[2], "r");
	//check the key is big enough
	if(verify_key_size(text, key)){
		fprintf(stderr, "CLIENT: key too short.\n");
		exit(1);
	}
	//check both files to see if they are useable
	if(validate_file(text)){
		fprintf(stderr, "CLIENT: plaintext has bad characters or does not exist.\n");
		exit(1);
	}
	if(validate_file(key)){
		fprintf(stderr, "CLIENT: cipher key has bad characters or does not exist.\n");
		exit(1);
	}

	// Clear out the address struct
	memset((char*) &address, '\0', sizeof(address));

	// The address should be network capable
	address.sin_family = AF_INET;

	// Store the port number
	portNumber = atoi(argv[3]);
	address.sin_port = htons(portNumber);

	// Get the DNS entry for this host name
	hostInfo = gethostbyname("LOCALHOST");
	if (hostInfo == NULL) { 
		fprintf(stderr, "CLIENT: ERROR, no such host\n");
		exit(1);
	}

	// Copy the first IP address from the DNS entry to sin_addr.s_addr
	memcpy(&address.sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);

	// Print the IP address in the socket address as a dotted decimal string
	// fprintf(stderr, "CLIENT: IP address for %s: %s\n", argv[1], inet_ntoa(address.sin_addr));


	//create socket at address of LOCALHOST
	if((server_socket_FD = socket(PF_INET, SOCK_STREAM, 0)) < 0){
		fprintf(stderr, "CLIENT: failed to create socket\n");
		exit(1);
	}
	//connect
	connect(server_socket_FD, (struct sockaddr*)&address, addrlen);

	//check the server is enc and not dec
	handshake(server_socket_FD);

	//send text and key
	send_file(server_socket_FD, text);
	send_file(server_socket_FD, key);

	//get encoded file
	char* cipher_text = get_file(server_socket_FD);

	//print to stdout
	printf("%s\n", cipher_text);

	//exit cleanly
	free(cipher_text);
	fclose(text);
	fclose(key);
	close(server_socket_FD);


	return 0;

}

//reads through a file and sends to the socket
void send_file(int server_socket_FD, FILE* file){
	char buffer[BUFFER_LEN] = {0};
	while(!feof(file)){
		memset(buffer, 0, BUFFER_LEN);
		fread(buffer, sizeof(char), BUFFER_LEN - 1, file);
		send(server_socket_FD, buffer, BUFFER_LEN, 0);
	}
	return;
}

//gets with recv and loads into string. returns string.
char* get_file(int server_socket_FD){
	int packets_recved = 0;
	char buffer[BUFFER_LEN] = {0};
	char* payload = calloc(0, sizeof(char));
	while(1){
		memset(buffer, 0, BUFFER_LEN);
		recv(server_socket_FD, buffer, BUFFER_LEN, 0);
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

int verify_key_size(FILE* text, FILE* key){
	fseek(text, 0, SEEK_END);
	fseek(key, 0, SEEK_END);
	if(ftell(key) < ftell(text)){
		return 1;
	}
	fseek(text, 0, SEEK_SET); // seek back to beginning of file
	fseek(key, 0, SEEK_SET); // seek back to beginning of file
	return 0;
}

int validate_file(FILE* file){
	char buffer[BUFFER_LEN];
	while(!feof(file)){
		memset(buffer, 0, BUFFER_LEN);
		fread(buffer, sizeof(char), BUFFER_LEN - 1, file);
		for(int i = 0; i < strlen(buffer); i++){
			if((int)buffer[i] != 10 && (int)buffer[i] != 32 && 
			((int)buffer[i] < 65 || (int)buffer[i] > 90)){
				return 1;
			}
		}
	}
	fseek(file, 0, SEEK_SET); // seek back to beginning of file
	return 0;
}

//sends SERVER_CODE and recieves server code. if this is bad then the connection is severed.
void handshake(int server_socket_FD){
	//send DESIRED_SERVER
	char buffer[BUFFER_LEN] = {0};
	send(server_socket_FD, SERVER_CODE, CODE_LEN, 0);
	//get response
	while(1){
		recv(server_socket_FD, buffer, BUFFER_LEN, 0);
		if(strlen(buffer) < BUFFER_LEN - 1 && strlen(buffer) != 0){
			break;
		}

	}
	if(strcmp(buffer, SERVER_CODE) != 0){
		close(server_socket_FD);
		exit(2);
	}
}