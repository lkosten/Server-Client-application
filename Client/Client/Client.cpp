#include "Client.h"


void Client::readData()
{
  std::wifstream inputFile;
  inputFile.open(commandsFileName);
  if (!inputFile)
  {
    std::cerr << "Can't open file with commands!" << std::endl;
    exit(0);
  }

  while (!inputFile.eof())
  {
    std::wstring command;
    inputFile >> command;
    commands.push_back(command);
  }

  inputFile.close();
}

void Client::tuneNetwork()
{
  WSAData wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
  {
    std::cerr << "Can't initialize winsock!" << std::endl;
    exit(0);
  }

  clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSocket == INVALID_SOCKET)
  {
    std::cerr << "Initializing socket failed with error: " << WSAGetLastError() << std::endl;
    WSACleanup();
    exit(0);
  }

  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(port);
  inet_pton(AF_INET, serverIpAddress.c_str(), &hint);

  if (connect(clientSocket, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR)
  {
    std::cerr << "Failed to connect to server with error: " << WSAGetLastError() << std::endl;
    closesocket(clientSocket);
    WSACleanup();
    exit(0);
  }
}

Client::Client(size_t requestNumber) : requestNumber(requestNumber)
{

}

Client::~Client()
{
  closesocket(clientSocket);
  WSACleanup();
}
