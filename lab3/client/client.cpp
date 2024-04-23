#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <sstream>
#include <cmath>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

double Function(double x) {
return 1.0 / log(x);
}

double CalculateIntegral(double start, double end, double step) {
double sum = 0.0;
for (double x = start; x < end; x += step) {
sum += Function(x) * step;
}
return sum;
}

void CalculateAndSendResult(SOCKET clientSocket, double start, double end, double step) {
double result = CalculateIntegral(start, end, step);

std::ostringstream resultStr;
resultStr << "Result-" << result;

send(clientSocket, resultStr.str().c_str(), resultStr.str().size() + 1, 0);
}

int main() {
WSADATA wsaData;
setlocale(LC_ALL, "en_us");
if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
std::cerr << "WSAStartup failed\n";
return 1;
}

std::string serverIP;
std::cout << "Enter server IP: ";
std::cin >> serverIP;

int port;
std::cout << "Enter server port: ";
std::cin >> port;

SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
if (clientSocket == INVALID_SOCKET) {
std::cerr << "Socket creation failed\n";
WSACleanup();
return 1;
}

sockaddr_in serverAddr{};
serverAddr.sin_family = AF_INET;
inet_pton(AF_INET, serverIP.c_str(), &(serverAddr.sin_addr));
serverAddr.sin_port = htons(port);

if (connect(clientSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
std::cerr << "Connection failed\n";
closesocket(clientSocket);
WSACleanup();
return 1;
}

std::cout << "Connected to server\n";

char paramsBuffer[1024];
char paramsBuffer1[1024];
recv(clientSocket, paramsBuffer, sizeof(paramsBuffer), 0);

double start1, end1, step1;
std::istringstream paramsStream(paramsBuffer);
paramsStream >> start1 >> end1 >> step1;

std::cout << "Received first integral parameters from server: start=" << start1 << ", end=" << end1 << ", step=" << step1 << "\n";

recv(clientSocket, paramsBuffer1, sizeof(paramsBuffer1), 0);
std::istringstream paramsStream1(paramsBuffer1);
double start2, end2, step2;
paramsStream1 >> start2 >> end2 >> step2;

std::cout << "Received second integral parameters from server: start=" << start2 << ", end=" << end2 << ", step=" << step2 << "\n";

std::vector<std::thread> threads;

unsigned int numThreads = std::thread::hardware_concurrency();

threads.reserve(numThreads);
threads.emplace_back(CalculateAndSendResult, clientSocket, start1, end1, step1);
threads.emplace_back(CalculateAndSendResult, clientSocket, start2, end2, step2);

for (auto& thread : threads) {
thread.join();
}

closesocket(clientSocket);
WSACleanup();

return 0;
}