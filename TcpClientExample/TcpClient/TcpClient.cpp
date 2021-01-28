#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <windows.h>
#include <signal.h>
#include <thread>

#pragma comment (lib, "Ws2_32.lib")
#define NEW_LINE "\n"


HANDLE h = GetStdHandle(STD_INPUT_HANDLE);
DWORD fdwMode;
void shutdownSocket(SOCKET);
bool validateIpAddress(const std::string);
void receiver(SOCKET, HANDLE);

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

	CloseHandle(h);
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

	std::thread receiverThread = std::thread(&receiver, sock, h);

	while (true)
	{
		int sendResult;
		std::cout << "> ";
		std::string userInput;
		std::getline(std::cin, userInput);
		if (userInput == "quit") {
			shutdownSocket(sock);
			break;
		}
		else if (userInput.size() > 0) {
			sendResult = send(sock, (userInput + NEW_LINE).c_str(), userInput.size() + 1, 0);
		}
		else {
			shutdownSocket(sock);
			break;
		}
	}
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

void receiver(SOCKET sock, HANDLE h) {
	while (true)
	{
		int iError;
		char recvbuf[1024] = "";
		int bytesRecv = recv(sock, recvbuf, 1024, 0);
		//std::cout << "bytesRecv : " << bytesRecv << "\n";
		if (bytesRecv > 0)
		{
			std::cout << "server: " << recvbuf << "\n";
			std::cout << "> ";
		}
		// bazý durumlarda  ctrl + c sinyali ile bytesRecv 0 yada -1 olabiliyor 
		// bu durum mutex_lock ile çözülebilir mi
		if (bytesRecv == 0) {
			std::cout << "Server is down !" << "\n";
			shutdownSocket(sock);
			break;
		}
		if (bytesRecv == -1) {
			iError = WSAGetLastError();
			std::cout << "Connection closed" << "\n";
			// eðer iError 10004 ise WSAEINTR
			// https://docs.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-wsacancelblockingcall
			// Interrupted function call. A blocking operation was interrupted by a call to WSACancelBlockingCall.
			//std::cout << iError << "\n";
			shutdownSocket(sock);
			break;
		}
	}
	fclose()
	std::cout << "thread is terminated" << "\n";
}