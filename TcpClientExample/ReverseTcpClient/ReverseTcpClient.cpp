#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <windows.h>
#include <signal.h>
#include <thread>

#pragma comment (lib, "Ws2_32.lib")

void sender(SOCKET);

volatile int signalNumber = 0;
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
	signalNumber = signum;
}

int main(int argc, char* argv[])
{

	char* ipAddress = NULL;
	int port = 0;

	if (argc == 3) {
		ipAddress = argv[1];
		port = atoi(argv[2]);
	}
	else
	{
		printf("Usage %s Ip port\n", argv[0]);
		printf("Terminating");
		return 1;
	}


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
		WSACleanup();
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
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	std::thread receiverThread = std::thread(&sender, sock);

	while (true)
	{
		int iError;
		char recvbuf[1024] = "";
		int bytesRecv = recv(sock, recvbuf, 1024, 0);
		if (bytesRecv > 0)
		{
			printf("Server: %s\n", recvbuf);
			printf("> ");
		}
		else if (bytesRecv == 0) {
			printf("Server is down !\n");
			break;
		}
		else if (bytesRecv == -1) {
			iError = WSAGetLastError();
			printf("Connection closed ERROR: %d\n ", iError);
			break;
		}
	}
	
	if (signalNumber == 0) {
		signalNumber = -1;
		HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
		CloseHandle(h);
	}

	receiverThread.join();
	printf("Terminated\n");
	return 0;
}

void sender(SOCKET sock) {
	char buff[1024];
	while (true)
	{

		int sendResult;
		printf("> ");
		if (!fgets(buff, 1023, stdin)) {
			break;
		}
		if (!strcmp(buff, "quit\n")) {
			break;
		}
		else if (strlen(buff) > 1) {
			sendResult = send(sock, buff, strlen(buff), 0);
		}
	}
	shutdown(sock, SD_BOTH);
	printf("thread is terminated\n");
}