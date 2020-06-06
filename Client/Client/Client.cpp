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

Client::Client() : commands(), commandQueue()
{
  readData();
  tuneNetwork();

  commandSended = CreateEvent(NULL, false, false, NULL);
  if (commandSended == NULL)
  {
    std::cerr << "Failed creating event!" << std::endl;
    exit(0);
  }
}

Client::~Client()
{
  CloseHandle(commandSended);
  closesocket(clientSocket);
  WSACleanup();
}

void Client::runClient(size_t requestNumber)
{
  for (size_t iteration = 0; iteration < requestNumber; ++iteration)
  {
    auto command = commands[iteration % commands.size()];

    int result;
    result = send(clientSocket, (char*)command.size(), sizeof(size_t), 0);
    if (result == 0)
    {
      std::cout << "Server disconnected." << std::endl;
      break;
    }
    else if (result < 0)
    {
      std::cerr << "send() failed with error: " << WSAGetLastError() << std::endl;
      break;
    }
    
    result = send(clientSocket, (char*)command.c_str(), sizeof(wchar_t) * command.size(), 0);
    if (result == 0)
    {
      std::cout << "Server disconnected." << std::endl;
      break;
    }
    else if (result < 0)
    {
      std::cerr << "send() failed with error: " << WSAGetLastError() << std::endl;
      break;
    }

    commandQueue.push(command);
    SetEvent(commandSended);
  }
}
