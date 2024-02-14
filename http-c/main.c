#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>

#define STYLESHEET "../concrete.css"
#define INDEX "../index.html"
#define ERROR_404 "HTTP/1.1 404 Not found\nContent-Type: text/html\n\n404 Not found"
#define ERROR_400 "HTTP/1.1 400 Bad request\nContent-Type: text/html\n\n400 Bad request"
#define PORT 1000

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
char* read_file(const char* path) {
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
// flush and close connection
void close_conn(SOCKET client) {
	shutdown(client, SD_SEND);
	closesocket(client);
}
// convert 32-bit int to ipv4 string
char* itoipv4(unsigned int ip_int) {
	char* ip_str = check_ptr(malloc(16));
	sprintf(ip_str, "%u.%u.%u.%u",
		ip_int & 0xFF,
		(ip_int >> 8) & 0xFF,
		(ip_int >> 16) & 0xFF,
		(ip_int >> 24) & 0xFF);
	return ip_str;
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
		.sin_port	= htons(PORT),
		.sin_addr	= 0,
	};

	// bind socket to address
	bind(listen_socket, (struct sockaddr*)&addr, sizeof(addr));

	listen(listen_socket, 20);

	// http loop
	while (true) {
		SOCKET client = accept(listen_socket, NULL, NULL);

		// Get client IP address
		struct sockaddr_in client_addr;
		int client_addr_len = sizeof(client_addr);
		if (getpeername(client, (struct sockaddr*)&client_addr, &client_addr_len) == SOCKET_ERROR)
			printf("Getpeername failed.\n");

		// receive message from client
		char msg[256];
		if (recv(client, msg, 256, 0) == -1) {
			printf("Buffer overflow on request message.\n");
			send(client, ERROR_400, sizeof(ERROR_400), 0);
			close_conn(client);
			continue;
		}

		// parse request type
		size_t req_type_len = strchr(msg, ' ') - msg;
		char* req_type = check_ptr(malloc(req_type_len + 1));
		substr(req_type, msg, req_type_len);
		if (!req_type) {
			printf("Error parsing Request.\n");
			send(client, ERROR_400, sizeof(ERROR_400), 0);
			close_conn(client);
			continue;
		}

		// parse file name
		size_t file_name_len = strchr(strchr(msg, '/') + 1, ' ') - (strchr(msg, '/') + 1);
		char* file_name = check_ptr(malloc(file_name_len + 1));
		substr(file_name, msg + req_type_len + 2, file_name_len);
		if (!file_name) {
			printf("Error parsing Request\n");
			send(client, ERROR_400, sizeof(ERROR_400), 0);
			close_conn(client);
			continue;
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
			char* file_cont = read_file(file_path);
			// file not found handling
			if (!file_cont) {
				printf("Failed to open file: %s\n", file_path);
				send(client, ERROR_404, sizeof(ERROR_404), 0);
				close_conn(client);
				continue;
			}

			// print client request info
			char* ip_str = itoipv4(client_addr.sin_addr.S_un.S_addr);
			printf("GET request: %s\n", ip_str);
			

			// send response
			char* headers = base_headers(file_path);
			send(client, headers, (int)strlen(headers), 0);
			send(client, file_cont, (int)strlen(file_cont), 0);

			close_conn(client);

			// free memory
			free(req_type);
			free(file_cont);
			free(file_name);
			free(ip_str);
		}
	}

	// resource cleanup
	closesocket(listen_socket);
	WSACleanup();

	return 0;
}