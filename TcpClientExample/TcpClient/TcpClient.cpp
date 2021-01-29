#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <windows.h>
#include <signal.h>
#include <thread>

#pragma comment (lib, "Ws2_32.lib")

void shutdownSocket(SOCKET);
bool validateIpAddress(const std::string);
void receiver(SOCKET);

int S = 0;
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
	
	std::string ipAddress;
	int port = 0;
	
	//i = 1  first parameter is executable name
	if (argc == 3) {

		for (int i = 1; i < argc; i++)
		{
			if (validateIpAddress(argv[i]))
			{
				ipAddress = argv[i];
				std::cout << "ip addr is valid " << ipAddress << "\n";
			}
			else if ((atoi(argv[i]) > 0 && atoi(argv[i]) < 65535))
			{
				port = atoi(argv[i]);
				std::cout << "port is valid " << port << "\n";
			}
			else {
				std::cout << "Unexpected argument" << "\n";
				std::cout << "Terminating ";
				exit(1);
			}

		}
	}
	else
	{
		std::cout << "(Need arguments 2 ip and port)" << "\n";
		std::cout << "Terminating ";
		exit(0);
	}


	/*std::string ipAddress = "127.0.0.1";
	int port = 1453;*/

	signal(SIGINT, signal_callback_handler);

	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0) {
		std::cerr << "Cant start winsock, ERR #" << wsResult << "\n";
		return 1;
	}

	//create socket
	SOCKET sock;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cerr << "Cant create socket, ERR #" << WSAGetLastError() << "\n";
		return 1;
	}
	//fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	int connResult = connect(sock, (sockaddr*)& hint, sizeof(hint));
	if (connResult == SOCKET_ERROR) {
		std::cerr << "Cant connect server, ERR #" << WSAGetLastError() << "\n";
		shutdownSocket(sock);
		WSACleanup();
		return 1;
	}

	std::thread receiverThread = std::thread(&receiver, sock);
	char buff[1024];
	while (true)
	{
		int sendResult;
		std::cout << "> ";
		if (!fgets(buff, 1023, stdin)) {
			break;
		}
		//std::getline(std::cin, userInput);
		if (!strncmp("quit", buff, 4)) {
			break;
		}
		else if (strlen(buff) > 0) {
			sendResult = send(sock, buff, strlen(buff), 0);
		}
		else {
			if (S != 0)
				break;
			//shutdownSocket(sock);
			//break;
		}
		printf("buffer %s", buff);
	}
	
	shutdown(sock, SD_BOTH);
	receiverThread.join();
	// Gracefully close down everything
	std::cout << "Terminated" << "\n";
	return 0;

}

void shutdownSocket(SOCKET sock) {
	shutdown(sock, SD_BOTH);
	WSACleanup();
}


bool validateIpAddress(const std::string ipAddress)
{
	struct sockaddr_in sa;
	int result = inet_pton(AF_INET, ipAddress.c_str(), &(sa.sin_addr));
	return result != 0;
}

void receiver(SOCKET sock) {
	while (true)
	{
		int iError;
		char recvbuf[1024] = "";
		int bytesRecv = recv(sock, recvbuf, 1024, 0);
		if (bytesRecv > 0)
		{
			std::cout << "server: " << recvbuf << "\n";
			std::cout << "> ";
		}
		// bazý durumlarda  ctrl + c sinyali ile bytesRecv 0 yada -1 olabiliyor 
		// bu durum mutex_lock ile çözülebilir mi
		if (bytesRecv == 0) {
			std::cout << "Server is down !" << "\n";
			break;
		}
		if (bytesRecv == -1) {
			iError = WSAGetLastError();
			std::cout << "Connection closed: "<<iError << "\n";
			break;
		}

	}

	if (S == 0) {
		S = -1;
		HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
		CloseHandle(h);
	}

	std::cout << "thread is terminated" << "\n";
}