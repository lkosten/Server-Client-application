#include "Server.h"

void Server::readData(std::unordered_map<std::wstring, std::wstring> &commandToResponse)
{
  std::wifstream inputFile;
  inputFile.open(dataFileName);
  if (!inputFile)
  {
    std::cerr << "Can't open file with commands!" << std::endl;
    exit(0);
  }

  while (!inputFile.eof())
  {
    std::wstring command, response;
    inputFile >> command >> response;
    commandToResponse[command] = response;
  }

  inputFile.close();
}

void Server::tuneNetwork()
{
  WSAData wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
  {
    std::cerr << "Can't initialize winsock!" << std::endl;
    exit(0);
  }

  listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket == INVALID_SOCKET)
  {
    std::cerr << "Initializing listening socket failed with error: " << WSAGetLastError() << std::endl;
    WSACleanup();
    exit(0);
  }

  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(port);
  hint.sin_addr.S_un.S_addr = INADDR_ANY;

  if (bind(listenSocket, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR)
  {
    std::cerr << "Binid" << std::endl;
    closesocket(listenSocket);
    WSACleanup();
    exit(0);
  }
  if (listen(listenSocket, maxConnections) == SOCKET_ERROR)
  {
    std::cerr << "Binding listening socket failed with error: " << WSAGetLastError() << std::endl;
    closesocket(listenSocket);
    WSACleanup();
    exit(0);
  }
}

DWORD __stdcall Server::listeningSocket(const LPVOID lpvParam)
{
  return 0;
}


Server::Server() : commandQueue(), handlerInfo(maxConnections)
{
  commandPushed = CreateEvent(NULL, false, false, NULL);
  if (commandPushed == NULL)
  {
    std::cerr << "Failed creating event!" << std::endl;
    exit(0);
  }

  for (size_t i = 0; i < maxConnections; ++i)
  {
    handlerInfo[i].index = i;
    handlerInfo[i].responsePushed = CreateEvent(NULL, false, false, NULL);
    if (handlerInfo[i].responsePushed == NULL)
    {
      std::cerr << "Failed creating commandPushed event!" << std::endl;
      exit(0);
    }
  }

  tuneNetwork();
}

Server::~Server()
{
  CloseHandle(commandPushed);
  for (auto &element : handlerInfo) CloseHandle(element.responsePushed);

  closesocket(listenSocket);
  WSACleanup();
}


void Server::runServer()
{
  std::unordered_map<std::wstring, std::wstring> commandToResponse;
  readData(commandToResponse);
  

  HANDLE listeningSocketThread = CreateThread(NULL, 0, listeningSocket, NULL, 0, NULL);
  if (listeningSocketThread == NULL)
  {
    std::cerr << "Failed creating listeningSocket thread!" << std::endl;
    exit(0);
  }
  else CloseHandle(listeningSocketThread);

  while (true)
  {
    if (commandQueue.empty()) WaitForSingleObject(commandPushed, INFINITE);

    auto curCommand = commandQueue.front();
    commandQueue.pop();

    handlerInfo[curCommand.second].responseQueue.push(commandToResponse[curCommand.first]);
    SetEvent(handlerInfo[curCommand.second].responsePushed);
  }
}

