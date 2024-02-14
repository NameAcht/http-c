#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>

#define STYLESHEET "../concrete.css"
#define INDEX "../index.html"

// platform
#ifdef _WIN32
# include <WinSock2.h>
#else
# include <sys/socket.h>
#endif

void* check_ptr(void* data) {
	if (!data) {
		printf("Out of memory.\n");
		exit(69);
	}
	return data;
}
void substr(char* dst, const char* src, size_t size) {
	memcpy(dst, src, size);
	dst[size] = '\0';
}
// make sure to free this pointer
char* parse_file(const char* path) {
	FILE* file = fopen(path, "r");
	if (!file)
		return 0;
	fseek(file, 0, SEEK_END);
	long fsize = ftell(file);
	rewind(file);

	char* file_cont = check_ptr(malloc(fsize + 1));
	fread(file_cont, fsize, 1, file);
	file_cont[fsize] = '\0';
	fclose(file);
	return file_cont;
}
char* base_headers(const char* path) {
	if (strstr(path, ".html"))
		return "HTTP/1.1 200 OK\nContent-Type: text/html\n\n";
	if (strstr(path, ".css"))
		return "HTTP/1.1 200 OK\nContent-Type: text/css\n\n";
	return NULL;
}

int main() {
	// WinSock boilerplate
	WSADATA wsaData;
	int init = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (init != NO_ERROR) {
		printf("Startup Error: %ld\n", init);
		return 0;
	}

	// create socket
	SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_socket == INVALID_SOCKET) {
		printf("Invalid listen socket.\n");
		return 0;
	}

	// create socket address
	struct sockaddr_in addr = {
		.sin_family	= AF_INET,
		.sin_port	= htons(0x3E8),
		.sin_addr	= 0,
	};

	// bind socket to address
	bind(listen_socket, (struct sockaddr*)&addr, sizeof(addr));

	listen(listen_socket, 20);

	while (true) {
		SOCKET client = accept(listen_socket, NULL, NULL);

		// receive message from client
		char msg[256];
		recv(client, msg, 256, 0);

		// parse request type
		int req_type_len = strchr(msg, ' ') - msg;
		char* req_type = check_ptr(malloc(req_type_len + 1));
		substr(req_type, msg, req_type_len);
		if (!req_type) {
			printf("Error parsing Request.\n");
			return 0;
		}

		// parse file name
		int file_name_len = strchr(strchr(msg, '/') + 1, ' ') - (strchr(msg, '/') + 1);
		char* file_name = check_ptr(malloc(file_name_len + 1));
		substr(file_name, msg + req_type_len + 2, file_name_len);
		if (!file_name) {
			printf("Error parsing Request\n");
			return 0;
		}
		// index.html default
		if (strlen(file_name) == 0) {
			file_name = check_ptr(realloc(file_name, 11));
			strcpy(file_name, "index.html");
		}

		char* file_path = check_ptr(malloc(strlen(file_name) + 4));
		strcpy(file_path, "../");
		strcpy(file_path + 3, file_name);

		// handle requests
		if (strcmp(req_type, "GET") == 0) {
			// parse file content
			char* file_cont = parse_file(file_path);
			if (!file_cont) {
				printf("Failed to open file: %s\n", file_path);
				return 0;
			}

			printf("GET request: ");

			// send response
			char* headers = base_headers(file_path);
			send(client, headers, strlen(headers), 0);
			send(client, file_cont, strlen(file_cont), 0);

			// flush and close connection
			shutdown(client, SD_SEND);
			closesocket(client);

			// free memory
			free(req_type);
			free(file_cont);
			free(file_name);
		}
	}

	// resource cleanup
	closesocket(listen_socket);
	WSACleanup();

	return 0;
}