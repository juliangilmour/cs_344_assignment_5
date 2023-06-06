compile with make

run with    ./enc_server PORT
	./dec_server PORT
	./enc_client TEXT KEY PORT
	./dec_client TEXT KEY PORT


fourth file does not decode. something to do with it being bigger than the buffer
I'm using. not enough time to figure out why.