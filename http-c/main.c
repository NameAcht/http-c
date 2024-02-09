#include <string.h>
#include <stdio.h>
#include <fcntl.h>

// platform
#ifdef _WIN32
# include <WinSock2.h>
#else
# include <sys/socket.h>
#endif

int main() {
	// create socket
	SOCKET server = socket(AF_INET, SOCK_STREAM, 0);

	// create socket address
	struct sockaddr_in addr = {
		.sin_family	= AF_INET,
		.sin_port	= 0x8E30,
		.sin_addr	= 0,
	};

	// bind socket to address
	bind(server, &addr, sizeof(addr));

	listen(server, 20);
	
	// get client file descriptor
	SOCKET client = accept(&server, 0, 0);

	// receive message from client
	char msg[256];
	recv(client, msg, 256, 0);

	// parse request type
	char* reqType = malloc(strchr(msg, ' ') - msg);
	if (!reqType) {
		printf("Out of memory.");
		return 0;
	}
	if (!strcpy_s(reqType, strchr(msg, ' ') - msg, msg)) {
		printf("Error parsing Request.");
		return 0;
	}

	// parse file name
	char* fileName = malloc(strchr(msg, '\n') - strlen(reqType));
	if (!fileName) {
		printf("Out of memory.");
		return 0;
	}
	if (!strcpy_s(fileName, strchr(msg, '\n') - strlen(reqType), msg + strlen(reqType))) {
		printf("Error parsing Request");
		return 0;
	}

	// handle requests
	if (reqType == "GET") {
		FILE* file = fopen(fileName, "r");
		if (!file) {
			printf("Failed to open file.");
			return 0;
		}
	}

	return 0;
}