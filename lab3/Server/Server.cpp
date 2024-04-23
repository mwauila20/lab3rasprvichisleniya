#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <thread>
#include <chrono>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

double Function(double x) {
	return 1.0 / log(x);
}

struct IntegrationParams {
	double start;
	double end;
	double step;

	IntegrationParams(double start, double end, double step)
		: start(start), end(end), step(step) {}
};

int main() {
	std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
	double accumulatedResult = 0.0;
	WSADATA wsaData;
	setlocale(LC_ALL, "en_us");
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		std::cerr << "WSAStartup failed\n";
		return 1;
	}

	int port;
	std::cout << "Enter server port: ";
	std::cin >> port;

	SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (serverSocket == INVALID_SOCKET) {
		std::cerr << "Socket creation failed\n";
		WSACleanup();
		return 1;
	}

	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
		std::cerr << "Bind failed\n";
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
		std::cerr << "Listen failed\n";
		closesocket(serverSocket);
		WSACleanup();
		return 1;
	}

	std::cout << "Server listening on port " << port << "\n";

	std::vector<SOCKET> clients;
	std::vector<IntegrationParams> integrationParams;
	fd_set readSet;
	char buffer[1024];
	int bytesRead;

	while (true) {
		FD_ZERO(&readSet);
		FD_SET(serverSocket, &readSet);

		for (const auto& client : clients) {
			FD_SET(client, &readSet);
		}

		int activity = select(0, &readSet, nullptr, nullptr, nullptr);
		if (activity == SOCKET_ERROR) {
			std::cerr << "Select failed\n";
			break;
		}

		if (FD_ISSET(serverSocket, &readSet)) {

			SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
			if (clientSocket == INVALID_SOCKET) {
				std::cerr << "Accept failed\n";
				break;
			}

			clients.push_back(clientSocket);

			char clientIP[INET_ADDRSTRLEN];
			sockaddr_in clientAddr;
			int clientAddrSize = sizeof(clientAddr);
			getpeername(clientSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
			inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);

			std::cout << "Client " << clients.size() << " connected: " << clientIP << ":" << ntohs(clientAddr.sin_port) << "\n";
			double step = 0.0000001;

			if (clients.size() == 2) {
				double range = 10.0;
				double step1 = ((20.0 - 10.0) / clients.size()) / 2;
				double start = 7.5;
				double end;
				int num_intervals = clients.size() * 2;
				int b1 = 0;
				int y = 0;
				for (size_t i = 0; i < num_intervals; ++i) {
					
					
					
					if (b1==2)
					{
						b1 = 100;
						y++;
					}

					start += step1;
					end = start + step1;
					integrationParams.clear();
					IntegrationParams params(start, end, step);
					integrationParams.push_back(params);

					std::ostringstream paramsStream;
					paramsStream << params.start << " " << params.end << " " << params.step;
					send(clients[y], paramsStream.str().c_str(), paramsStream.str().size() + 1, 0);
					b1++;
					startTime = std::chrono::system_clock::now();
				}
			}

		}

		for (auto it = clients.begin(); it != clients.end(); ) {
			SOCKET clientSocket = *it;

			if (FD_ISSET(clientSocket, &readSet)) {
				bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
				if (bytesRead > 0) {
					buffer[bytesRead] = '\0';
					std::cout << "Received from client " << std::distance(clients.begin(), it) + 1 << ": " << buffer << "\n";

					if (strncmp(buffer, "Result-", 7) == 0) {
						double result = std::stod(buffer + 7);

						std::cout << "Received integral result from client " << std::distance(clients.begin(), it) + 1 << ": " << result << "\n";

						accumulatedResult += result;
						endTime = std::chrono::system_clock::now();
						std::chrono::duration<double> elapsed_seconds = endTime - startTime;
						std::cout << "Integration process took : " << elapsed_seconds.count() << " seconds\n";
						std::cout << "Accumulated result: " <<
							accumulatedResult << "\n";
					}
				}
				else {
					std::cout << "Client " << std::distance(clients.begin(), it) + 1 << " disconnected\n";
					closesocket(clientSocket);
					it = clients.erase(it);
					continue;
				}
			}

			++it;
		}
	}


	for (const auto& client : clients) {
		closesocket(client);
	}

	closesocket(serverSocket);
	WSACleanup();

	return 0;
}