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

volatile HANDLE h = GetStdHandle(STD_INPUT_HANDLE);

// do not use std::cout in signal handler
void signal_callback_handler(int signum) {
	CloseHandle(h);
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

	void shutdownSocket() {
		shutdown(this->sock, SD_BOTH);
		WSACleanup();
	}

	//socket getter setter
	SOCKET getSocket() {
		return this->sock;
	}

	void setSocket(SOCKET sock) {
		this->sock = sock;
	}

	//ipAddress getter setter
	std::string getIpAddress() {
		return this->ipAddress;
	}

	void setIpAddress(std::string ipAddress) {
		this->ipAddress = ipAddress;
	}


	// Port getter setter
	int getPort() {
		return this->port;
	}

	void setPort(int port) {
		this->port = port;
	}



private:
	WSAData data;
	WORD ver;
	SOCKET sock;
	sockaddr_in hint;
	std::string ipAddress;
	int port;

};


void receiver(MySocket& myTcpSocket) {
	std::cout << "DEBUG: receiver started." << "\n";
	while (h != INVALID_HANDLE_VALUE)
	{
		char recvbuf[1024] = "";
		int bytesRecv = recv(myTcpSocket.getSocket(), recvbuf, 1024, 0);
		//std::cout << "bytesRecv : " << bytesRecv << "\n";
		if (bytesRecv > 0)
		{
			std::cout << "server: " << recvbuf << "\n";
			std::cout << "> ";
		}
		if (bytesRecv == 0) {
			std::cout << "Server is down !" << "\n";
			myTcpSocket.shutdownSocket();
			break;
		}
		if (bytesRecv == -1) {
			myTcpSocket.shutdownSocket();
			break;
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
	myTcpSocket.setIpAddress(ipAddress);
	myTcpSocket.setPort(port);

	myTcpSocket.createSocket();
	myTcpSocket.connectSocket();

	std::thread receiverThread = std::thread(&receiver, std::ref(myTcpSocket));
	//CloseHandle(h);



	while (h != INVALID_HANDLE_VALUE)
	{
		int sendResult;
		std::cout << "> ";
		std::string userInput;
		std::getline(std::cin, userInput);
		if (userInput == "quit") {
			myTcpSocket.shutdownSocket();
			break;
		}
		else if (userInput.size() > 0) {
			sendResult = send(myTcpSocket.getSocket(), (userInput + NEW_LINE).c_str(), userInput.size() + 1, 0);
		}
		else {
			break;
		}
	}


	// Gracefully close down everything
	std::cout << "Terminated" << "\n";
	return 0;
}