#include "stdio.h"
#include <stdlib.h>
#include "time.h"
#include "string.h"

int main(int argc, char** argv){
	if(argc < 2)
		return 1;
	int len = atoi(argv[1]);
	if(len <= 0)
		return 1;
	int n;
	char c;
	for(int i = 0; i < len; i++){
		n = rand() % 27;
		if(n == 0)
			c = ' ';
		else
			c = (char)(n + 64);
		printf("%c", c);
	}
	printf("\n");
	return 0;
}