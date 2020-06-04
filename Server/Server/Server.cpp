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

DWORD __stdcall Server::listeningSocket(LPVOID lpvParam)
{
  std::tuple<std::queue<size_t>*, HANDLE, SOCKET, std::vector<HandlerInfo>*> *param = 
    (std::tuple<std::queue<size_t>*, HANDLE, SOCKET, std::vector<HandlerInfo>*>*)lpvParam;
  std::queue<size_t> &freeSocket = *std::get<0>(*param);
  HANDLE clientDisconnected = std::get<1>(*param);
  SOCKET listenSocket = std::get<2>(*param);
  std::vector<HandlerInfo> &handlerInfo = *std::get<3>(*param);
  delete param;

  while (true)
  {
    if (freeSocket.empty()) WaitForSingleObject(clientDisconnected, INFINITE);
    
    int index = freeSocket.front();
    freeSocket.pop();

    sockaddr_in client;
    int clientSize = sizeof(client);
    handlerInfo[index].clientSocket = accept(listenSocket, (sockaddr*)&client, &clientSize);
    if (handlerInfo[index].clientSocket == INVALID_SOCKET)
    {
      std::cerr << "Client connection failed with error: " << WSAGetLastError() << std::endl;
      continue;
    }

    HANDLE clientHandlerThread = CreateThread(NULL, 0, clientHandler, param, 0, NULL);
    if (clientHandlerThread == NULL)
    {
      std::cerr << "Failed creating clientHandler thread!" << std::endl;
      exit(0);
    }
    else CloseHandle(clientHandlerThread);
  }


  return 0;
}

DWORD __stdcall Server::clientHandler(LPVOID lpvParam)
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
  

  auto param = new std::tuple<std::queue<size_t>*, HANDLE, SOCKET, std::vector<HandlerInfo>*>
    (&freeSocket, clientDisconnected, listenSocket, &handlerInfo);
  HANDLE listeningSocketThread = CreateThread(NULL, 0, listeningSocket, param, 0, NULL);
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

