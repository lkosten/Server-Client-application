#include "Server.h"

void Server::readData(std::unordered_map<std::wstring, std::wstring> &commandToResponse)
{
  std::wifstream inputFile;
  inputFile.open(dataFileName);
  if (!inputFile)
  {
    EnterCriticalSection(&outputCriticalSection);
    std::cerr << "Can't open file with commands!" << std::endl;
    LeaveCriticalSection(&outputCriticalSection);
    system("pause");
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
    EnterCriticalSection(&outputCriticalSection);
    std::cerr << "Can't initialize winsock!" << std::endl;
    LeaveCriticalSection(&outputCriticalSection);
    system("pause");
    exit(0);
  }

  listenSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (listenSocket == INVALID_SOCKET)
  {
    EnterCriticalSection(&outputCriticalSection);
    std::cerr << "Initializing listening socket failed with error: " << WSAGetLastError() << std::endl;
    LeaveCriticalSection(&outputCriticalSection);
    WSACleanup();
    system("pause");
    exit(0);
  }

  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(port);
  hint.sin_addr.S_un.S_addr = INADDR_ANY;

  if (bind(listenSocket, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR)
  {
    EnterCriticalSection(&outputCriticalSection);
    std::cerr << "Binding socket failed with error: " << WSAGetLastError() << std::endl;
    LeaveCriticalSection(&outputCriticalSection);
    closesocket(listenSocket);
    WSACleanup();
    system("pause");
    exit(0);
  }
  if (listen(listenSocket, maxConnections) == SOCKET_ERROR)
  {
    EnterCriticalSection(&outputCriticalSection);
    std::cerr << "Listening socket failed with error: " << WSAGetLastError() << std::endl;
    LeaveCriticalSection(&outputCriticalSection);
    closesocket(listenSocket);
    WSACleanup();
    system("pause");
    exit(0);
  }
}

DWORD __stdcall Server::listeningSocket(const LPVOID lpvParam)
{
  Server &server = *(Server*)lpvParam;

  while (true)
  {
    if (server.freeSocket.empty())
    {
      ResetEvent(server.clientDisconnected);
      WaitForSingleObject(server.clientDisconnected, INFINITE);
    }
    
    int index = server.freeSocket.front();
    server.freeSocket.pop();

    sockaddr_in client;
    int clientSize = sizeof(client);
    server.handlerInfo[index].clientSocket = accept(server.listenSocket, (sockaddr*)&client, &clientSize);
    if (server.handlerInfo[index].clientSocket == INVALID_SOCKET)
    {
      EnterCriticalSection(&server.outputCriticalSection);
      std::cerr << "Client connection failed with error: " << WSAGetLastError() << std::endl;
      LeaveCriticalSection(&server.outputCriticalSection);
      system("pause");
      continue;
    }

    ZeroMemory(server.handlerInfo[index].host, NI_MAXHOST);
    ZeroMemory(server.handlerInfo[index].service, NI_MAXSERV);
    if (getnameinfo((sockaddr*)&client, sizeof(client),
      server.handlerInfo[index].host, NI_MAXHOST, server.handlerInfo[index].service, NI_MAXSERV, 0) == 0)
    {
      EnterCriticalSection(&server.outputCriticalSection);
      std::cout << server.handlerInfo[index].host << " connected on port " << server.handlerInfo[index].service << std::endl;
      LeaveCriticalSection(&server.outputCriticalSection);
    }
    else
    {
      EnterCriticalSection(&server.outputCriticalSection);
      std::cerr << "Can't get information from client: " << WSAGetLastError() << std::endl;
      LeaveCriticalSection(&server.outputCriticalSection);
    }

    auto handlerParam1 = new std::pair<Server*, size_t>(&server, index);
    auto handlerParam2 = new std::pair<Server*, size_t>(&server, index);
    HANDLE clientHandlerReceiverThread = CreateThread(NULL, 0, clientHandlerReceiver, handlerParam1, 0, NULL);
    if (clientHandlerReceiver == NULL)
    {
      EnterCriticalSection(&server.outputCriticalSection);
      std::cerr << "Failed creating clientHandlerReceiver thread!" << std::endl;
      LeaveCriticalSection(&server.outputCriticalSection);
      system("pause");
      exit(0);
    }
    else CloseHandle(clientHandlerReceiverThread);
    HANDLE clientHandlerSenderThread = CreateThread(NULL, 0, clientHandlerSender, handlerParam2, 0, NULL);
    if (clientHandlerSenderThread == NULL)
    {
      EnterCriticalSection(&server.outputCriticalSection);
      std::cerr << "Failed creating clientHandlerSender thread!" << std::endl;
      LeaveCriticalSection(&server.outputCriticalSection);
      system("pause");
      exit(0);
    }
    else CloseHandle(clientHandlerSenderThread);

  }


  return 0;
}

DWORD __stdcall Server::clientHandlerReceiver(const LPVOID lpvParam)
{
  Server &server = *((std::pair<Server*, size_t>*)lpvParam)->first;
  size_t index = ((std::pair<Server*, size_t>*)lpvParam)->second;
  delete lpvParam;

  while (true)
  {
    size_t commandLen;
    int result;
    result = recv(server.handlerInfo[index].clientSocket, (char*)&commandLen, sizeof(commandLen), 0);
    if (result == 0)
    {
      break;
    }
    else if (result < 0)
    {
      EnterCriticalSection(&server.outputCriticalSection);
      std::cerr << "recv() failed with error: " << WSAGetLastError() << std::endl;
      LeaveCriticalSection(&server.outputCriticalSection);
      break;
    }

    wchar_t *command = new wchar_t[commandLen];
    result = recv(server.handlerInfo[index].clientSocket, (char*)command, commandLen * sizeof(wchar_t), 0);
    if (result == 0)
    {
      break;
    }
    else if (result < 0)
    {
      EnterCriticalSection(&server.outputCriticalSection);
      std::cerr << "recv() failed with error: " << WSAGetLastError() << std::endl;
      LeaveCriticalSection(&server.outputCriticalSection);
      break;
    }

    server.commandQueue.emplace(command, index);
    SetEvent(server.commandPushed);
    delete[]command;

    EnterCriticalSection(&server.outputCriticalSection);
    auto temp = std::time(nullptr);
    char *timestamp = new char[200];
    tm locTime;
    localtime_s(&locTime, &temp);
    asctime_s(timestamp, 200, &locTime);
    std::wcout << timestamp;
    std::cout << "Command received from " <<
      server.handlerInfo[index].host << " on port " << server.handlerInfo[index].service << std::endl;
    LeaveCriticalSection(&server.outputCriticalSection);
    delete[]timestamp;

  }

  EnterCriticalSection(&server.outputCriticalSection);
  std::cout << "Client " << server.handlerInfo[index].host << " on port " << server.handlerInfo[index].service << " disonnected." << std::endl;
  LeaveCriticalSection(&server.outputCriticalSection);

  closesocket(server.handlerInfo[index].clientSocket);
  server.freeSocket.push(index);
  SetEvent(server.clientDisconnected);
  SetEvent(server.handlerInfo[index].responsePushed);
  return 0;
}

DWORD __stdcall Server::clientHandlerSender(const LPVOID lpvParam)
{
  Server &server = *((std::pair<Server*, size_t>*)lpvParam)->first;
  size_t index = ((std::pair<Server*, size_t>*)lpvParam)->second;
  delete lpvParam;

  while (true)
  {
    if (server.handlerInfo[index].responseQueue.empty())
    {
      ResetEvent(server.handlerInfo[index].responsePushed);
      WaitForSingleObject(server.handlerInfo[index].responsePushed, INFINITE);
    }
    if (server.handlerInfo[index].responseQueue.empty()) break;

    auto response = server.handlerInfo[index].responseQueue.front();
    server.handlerInfo[index].responseQueue.pop();

    size_t len = response.size() + 1;
    int result;
    result = send(server.handlerInfo[index].clientSocket, (char*)&len, sizeof(size_t), 0);
    if (result == 0)
    {
      break;
    }
    else if (result < 0)
    {
      EnterCriticalSection(&server.outputCriticalSection);
      std::cerr << "send() failed with error: " << WSAGetLastError() << std::endl;
      LeaveCriticalSection(&server.outputCriticalSection);
      break;
    }
    result = send(server.handlerInfo[index].clientSocket, (char*)response.c_str(), sizeof(wchar_t) * len, 0);
    if (result == 0)
    {
      break;
    }
    else if (result < 0)
    {
      EnterCriticalSection(&server.outputCriticalSection);
      std::cerr << "send() failed with error: " << WSAGetLastError() << std::endl;
      LeaveCriticalSection(&server.outputCriticalSection);
      break;
    }

    EnterCriticalSection(&server.outputCriticalSection);
    auto temp = std::time(nullptr);
    char *timestamp = new char[200];
    tm locTime;
    localtime_s(&locTime, &temp);
    asctime_s(timestamp, 200, &locTime);
    std::wcout << timestamp;
    std::cout << "Sended response for " <<
      server.handlerInfo[index].host << " on port " << server.handlerInfo[index].service << std::endl;
    LeaveCriticalSection(&server.outputCriticalSection);
    delete[]timestamp;
  }

  return 0;
}


Server::Server() : commandQueue(), handlerInfo(maxConnections)
{
  InitializeCriticalSection(&outputCriticalSection);
  commandPushed = CreateEvent(NULL, false, false, NULL);
  if (commandPushed == NULL)
  {
    EnterCriticalSection(&outputCriticalSection);
    std::cerr << "Failed creating event!" << std::endl;
    LeaveCriticalSection(&outputCriticalSection);
    system("pause");
    exit(0);
  }
  clientDisconnected = CreateEvent(NULL, false, false, NULL);
  if (clientDisconnected == NULL)
  {
    EnterCriticalSection(&outputCriticalSection);
    std::cerr << "Failed creating event!" << std::endl;
    LeaveCriticalSection(&outputCriticalSection);
    system("pause");
    exit(0);
  }

  for (size_t i = 0; i < maxConnections; ++i)
  {
    handlerInfo[i].index = i;
    handlerInfo[i].responsePushed = CreateEvent(NULL, false, false, NULL);
    if (handlerInfo[i].responsePushed == NULL)
    {
      EnterCriticalSection(&outputCriticalSection);
      std::cerr << "Failed creating commandPushed event!" << std::endl;
      LeaveCriticalSection(&outputCriticalSection);
      system("pause");
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
  DeleteCriticalSection(&outputCriticalSection);
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
    EnterCriticalSection(&outputCriticalSection);
    std::cerr << "Failed creating listeningSocket thread!" << std::endl;
    LeaveCriticalSection(&outputCriticalSection);
    system("pause");
    exit(0);
  }
  else CloseHandle(listeningSocketThread);

  while (true)
  {
    if (commandQueue.empty())
    {
      ResetEvent(commandPushed);
      WaitForSingleObject(commandPushed, INFINITE);
    }

    auto curCommand = commandQueue.front();
    commandQueue.pop();

    handlerInfo[curCommand.second].responseQueue.push(commandToResponse[curCommand.first]);
    SetEvent(handlerInfo[curCommand.second].responsePushed);
  }
}

