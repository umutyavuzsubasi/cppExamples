#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <windows.h>
#include <signal.h>
#include <thread>

#pragma comment (lib, "Ws2_32.lib")
#define NEW_LINE "\n"

/*
	Big endian -> Network Byte Order
	ntohs();
	ntohl();
	Litte endian -> Host Byte Order
	htons();
	htonl();
*/

volatile bool isRunning = true;


// do not use std::cout in signal handler
void signal_callback_handler(int signum) {
	isRunning = false;
}

bool validateIpAddress(const std::string& ipAddress)
{
	struct sockaddr_in sa;
	int result = inet_pton(AF_INET, ipAddress.c_str(), &(sa.sin_addr));
	return result != 0;
}

class MySocket
{
public:
	WSAData data;
	WORD ver;
	SOCKET sock;
	sockaddr_in hint;
	std::string ipAddress;
	int port;
	std::string status;


public:
	int createSocket()
	{
		ver = MAKEWORD(2, 2);
		int wsResult = WSAStartup(ver, &data);
		if (wsResult != 0) {
			std::cerr << "Cant start winsock, ERR #" << wsResult << "\n";
			exit(1);
		}

		//create socket
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == INVALID_SOCKET) {
			std::cerr << "Cant create socket, ERR #" << WSAGetLastError() << "\n";
			exit(1);
		}
		//fill in a hint structure
		hint.sin_family = AF_INET;
		hint.sin_port = htons(this->port);
		inet_pton(AF_INET, this->ipAddress.c_str(), &hint.sin_addr);
		return 1;
	}

	int connectSocket()
	{
		int connResult = connect(this->sock, (sockaddr*)& this->hint, sizeof(this->hint));
		if (connResult == SOCKET_ERROR) {
			std::cerr << "Cant connect server, ERR #" << WSAGetLastError() << "\n";
			closesocket(this->sock);
			WSACleanup();
			exit(1);
		}
		else {
			return 0;
		}

	}

	void runSocket()
	{
		this->status = "run";
	}

	int shutdownSocket() {
		std::cout << "Gracefully Exit " << "\n";
		closesocket(this->sock);
		WSACleanup();
		this->status = "shutdown";
		exit(1);
	}


	void reconnectSocket() {
		this->status = "reconnect";
		std::cout << "Reconnect " << "\n";
		std::cout << "DEBUG: reconnectSocket reconnect." << "\n";
		closesocket(this->sock);
		WSACleanup();
		this->createSocket();
		this->connectSocket();
	}


};


void receiver(MySocket& myTcpSocket) {
	std::cout << "DEBUG: receiver started." << "\n";
	while (1)
	{
		if (myTcpSocket.status == "run")
		{
			char recvbuf[1024] = "";
			int bytesRecv = recv(myTcpSocket.sock, recvbuf, 1024, 0);
			//std::cout << "bytesRecv : " << bytesRecv << "\n";
			if (bytesRecv > 0)
			{
				std::cout << "server: " << recvbuf << "\n";
				std::cout << "> ";
			}
			if (bytesRecv == 0) {
				std::cout << "Server is down !" << "\n";
				myTcpSocket.shutdownSocket();
			}
			if (bytesRecv == -1) {
				break;
			}
		}
		else if (myTcpSocket.status == "reconnect")
		{

		}

	}
}

void sent(MySocket& myTcpSocket) {
	std::cout << "DEBUG: sender started." << "\n";
	std::string userInput;
	while (1)
	{
		std::cout << "> ";
		std::getline(std::cin, userInput);
		if (userInput == "quit") {
			myTcpSocket.shutdownSocket();
		}
		if (userInput == "reset") {
			myTcpSocket.reconnectSocket();
		}
		else if (userInput.size() > 0) {
			int sendResult = send(myTcpSocket.sock, (userInput + NEW_LINE).c_str(), userInput.size() + 1, 0);
		}
	}
}



int main(int argc, char* argv[])
{
	std::cout << "Started \n";
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
		exit(1);
	}


	/*std::string ipAddress = "127.0.0.1";
	int port = 1453;*/

	signal(SIGINT, signal_callback_handler);


	MySocket myTcpSocket;
	myTcpSocket.ipAddress = ipAddress;
	myTcpSocket.port = port;

	myTcpSocket.createSocket();
	myTcpSocket.connectSocket();
	myTcpSocket.runSocket();

	auto receiverThread = std::thread(&receiver, std::ref(myTcpSocket));
	auto senderThread = std::thread(&sent, std::ref(myTcpSocket));

	// socket command tracer
	while (myTcpSocket.status != "shutdown") {

		if (myTcpSocket.status == "reconnect") {
			std::cout << "status is reconnect" << "\n";
			myTcpSocket.runSocket();
		}
		else if (myTcpSocket.status == "run")
		{
			// do nothing
		}

	}
	// Gracefully close down everything
	return 1;
}