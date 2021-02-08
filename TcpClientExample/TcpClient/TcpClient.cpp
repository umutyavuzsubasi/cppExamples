#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <windows.h>
#include <signal.h>
#include <thread>

#pragma comment (lib, "Ws2_32.lib")

void shutdownSocket(SOCKET);
void receiver(SOCKET);

static int S = 0;
/*
	Big endian -> Network Byte Order
	ntohs();
	ntohl();
	Litte endian -> Host Byte Order
	htons();
	htonl();
*/

// do not use std::cout in signal handler
void signal_callback_handler(int signum) {
	S = signum;
}

int main(int argc, char* argv[])
{
	
	char* ipAddress = NULL;
	int port = 0;
	//i = 1  first parameter is executable name
	if (argc == 3) {
		ipAddress = argv[1];
		port = atoi(argv[2]);
	}
	else
	{
		printf("Usage %s Ip port\n", argv[0]);
		printf("Terminating");
		exit(1);
	}


	/*std::string ipAddress = "127.0.0.1";
	int port = 1453;*/

	signal(SIGINT, signal_callback_handler);


	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0) {
		printf("Cant start winsock, ERROR: %d\n", wsResult);
		return 1;
	}

	//create socket
	SOCKET sock;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		printf("Cant create socket, ERROR: %d\n", WSAGetLastError());
		return 1;
	}
	//fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress, &hint.sin_addr);

	int connResult = connect(sock, (sockaddr*)& hint, sizeof(hint));
	if (connResult == SOCKET_ERROR) {
		printf("Cant connect server, ERROR: %d\n", WSAGetLastError());
		shutdownSocket(sock);
		WSACleanup();
		return 1;
	}

	std::thread receiverThread = std::thread(&receiver, sock);
	char buff[1024];
	while (true)
	{
		int sendResult;
		printf("> ");
		if (!fgets(buff, 1023, stdin)) {
			break;
		}
		//std::getline(std::cin, userInput);
		if (!strncmp("quit", buff, 4)) {
			break;
		}
		else if (strlen(buff) > 1) {
			sendResult = send(sock, buff, strlen(buff), 0);
		}
		else {
			if (S != 0)
				break;
		}
	}
	
	shutdown(sock, SD_BOTH);
	receiverThread.join();
	// Gracefully close down everything
	printf("Terminated\n");
	return 0;

}

void shutdownSocket(SOCKET sock) {
	shutdown(sock, SD_BOTH);
	WSACleanup();
}

void receiver(SOCKET sock) {
	while (true)
	{
		int iError;
		char recvbuf[1024] = "";
		int bytesRecv = recv(sock, recvbuf, 1024, 0);
		if (bytesRecv > 0)
		{
			printf("server: %s\n", recvbuf);
			printf("> ");
		}
		if (bytesRecv == 0) {
			printf("Server is down !\n");
			break;
		}
		if (bytesRecv == -1) {
			iError = WSAGetLastError();
			printf("Connection closed ERROR: %d\n ", iError);
			break;
		}

	}

	if (S == 0) {
		S = -1;
		HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
		CloseHandle(h);
	}

	printf("thread is terminated\n");
}