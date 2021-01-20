#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include <windows.h>
#include <signal.h>

#pragma comment (lib, "Ws2_32.lib")
#define NEW_LINE "\n"



volatile int isRunning = true;

void signal_callback_handler(int signum) {
	isRunning = false;
	std::cout << "Terminating with CTRL + C SIGNAL" << NEW_LINE;
}



int main(int argc, char* argv[])
{
	std::cout << "Started " << NEW_LINE;
	std::string ipAddress = argv[1];
	int port = std::atoi(argv[2]);

	/*std::string ipAddress = "127.0.0.1";
	int port = 1453;*/

	std::cout << "got ip addr : " << argv[1] << " and  got port : " << argv[2] << NEW_LINE;


	signal(SIGINT, signal_callback_handler);

	//initalize windows socket
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0) {
		std::cerr << "Cant start winsock, ERR #" << wsResult << NEW_LINE;
		return 1;
	}

	//create socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) {
		std::cerr << "Cant create socket, ERR #" << WSAGetLastError() << NEW_LINE;
		return 1;
	}
	//fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	//connect to server

	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR) {
		std::cerr << "Cant connect server, ERR #" << WSAGetLastError() << NEW_LINE;
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	// do while loop to send and receive data
	char buff[1024];
	std::string userInput;


	while(isRunning)
	{
		std::cout << "> ";
		std::getline(std::cin, userInput);
		if (userInput.size() > 0 && userInput != "quit") {
			int sendResult = send(sock, (userInput + NEW_LINE).c_str(), userInput.size() + 1, 0);
			std::cout << userInput << NEW_LINE;
		}
		if(userInput == "quit") {
			std::cout << "Terminating got quit input " << NEW_LINE;
			break;
		}
	}

	// Gracefully close down everything
	std::cout << "Gracefully shutdown everything" << NEW_LINE;
	closesocket(sock);
	WSACleanup();
	
	return 1;
}