CC=gcc
CFlags= -g -Wall --std=gnu99

all: enc_server dec_server enc_client dec_client keygen
# enc_server: enc_server.c
# 	$(CC) $(CFlags) -c enc_server.c
enc_server: enc_server.c
	$(CC) $(CFlags) -o enc_server enc_server.c
#dec_server: dec_server.c
#	$(CC) $(CFlags) -c dec_server.c
dec_server: dec_server.c
	$(CC) $(CFlags) -o dec_server dec_server.c
# enc_client: enc_client.c
# 	$(CC) $(CFlags) -c enc_client.c
enc_client: enc_client.c
	$(CC) $(CFlags) -o enc_client enc_client.c
# dec_client: dec_client.c
# 	$(CC) $(CFlags) -c dec_client.c
dec_client: dec_client.c
	$(CC) $(CFlags) -o dec_client dec_client.c
keygen: keygen.c
	$(CC) $(CFlags) -o keygen keygen.c

clean:
	-clear
	-rm enc_server dec_server enc_client dec_client