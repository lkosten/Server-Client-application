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
  Server &server = *(Server*)lpvParam;

  while (true)
  {
    if (server.freeSocket.empty()) WaitForSingleObject(server.clientDisconnected, INFINITE);
    
    int index = server.freeSocket.front();
    server.freeSocket.pop();

    sockaddr_in client;
    int clientSize = sizeof(client);
    server.handlerInfo[index].clientSocket = accept(server.listenSocket, (sockaddr*)&client, &clientSize);
    if (server.handlerInfo[index].clientSocket == INVALID_SOCKET)
    {
      std::cerr << "Client connection failed with error: " << WSAGetLastError() << std::endl;
      continue;
    }

    auto handlerParam = new std::pair<Server*, size_t>(&server, index);
    HANDLE clientHandlerThread = CreateThread(NULL, 0, clientHandlerReceiver, handlerParam, 0, NULL);
    if (clientHandlerThread == NULL)
    {
      std::cerr << "Failed creating clientHandler thread!" << std::endl;
      exit(0);
    }
    else CloseHandle(clientHandlerThread);
  }


  return 0;
}

DWORD __stdcall Server::clientHandlerReceiver(LPVOID lpvParam)
{
  Server &server = *((std::pair<Server*, size_t>*)lpvParam)->first;
  size_t index = ((std::pair<Server*, size_t>*)lpvParam)->second;


  while (true)
  {
    size_t commandLen;
    int result;
    result = recv(server.handlerInfo[index].clientSocket, (char*)&commandLen, sizeof(commandLen), 0);
    if (result == 0)
    {
      std::cout << "Client disonnected." << std::endl;
      break;
    }
    else if (result < 0)
    {
      std::cerr << "recv() failed with error: " << WSAGetLastError() << std::endl;
      break;
    }

    wchar_t *command = new wchar_t[commandLen];
    result = recv(server.handlerInfo[index].clientSocket, (char*)command, sizeof(commandLen) * sizeof(wchar_t), 0);
    if (result == 0)
    {
      std::cout << "Client disonnected." << std::endl;
      break;
    }
    else if (result < 0)
    {
      std::cerr << "recv() failed with error: " << WSAGetLastError() << std::endl;
      break;
    }

    server.commandQueue.emplace(command, index);
    SetEvent(server.commandPushed);
  }

  closesocket(server.handlerInfo[index].clientSocket);
  server.freeSocket.push(index);
  SetEvent(server.clientDisconnected);
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
  clientDisconnected = CreateEvent(NULL, false, false, NULL);
  if (clientDisconnected == NULL)
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

    freeSocket.push(i);
  }

  tuneNetwork();
}

Server::~Server()
{
  CloseHandle(commandPushed);
  CloseHandle(clientDisconnected);
  for (auto &element : handlerInfo) CloseHandle(element.responsePushed);

  closesocket(listenSocket);
  WSACleanup();
}


void Server::runServer()
{
  std::unordered_map<std::wstring, std::wstring> commandToResponse;
  readData(commandToResponse);
  

  HANDLE listeningSocketThread = CreateThread(NULL, 0, listeningSocket, this, 0, NULL);
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

