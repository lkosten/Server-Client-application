#include "Client.h"


void Client::readData()
{
  std::wifstream inputFile;
  inputFile.open(commandsFileName);
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
    EnterCriticalSection(&outputCriticalSection);
    std::cerr << "Can't initialize winsock!" << std::endl;
    LeaveCriticalSection(&outputCriticalSection);
    system("pause");
    exit(0);
  }

  clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if (clientSocket == INVALID_SOCKET)
  {
    EnterCriticalSection(&outputCriticalSection);
    std::cerr << "Initializing socket failed with error: " << WSAGetLastError() << std::endl;
    LeaveCriticalSection(&outputCriticalSection);
    WSACleanup();
    system("pause");
    exit(0);
  }

  sockaddr_in hint;
  hint.sin_family = AF_INET;
  hint.sin_port = htons(port);
  inet_pton(AF_INET, serverIpAddress.c_str(), &hint.sin_addr);

  if (connect(clientSocket, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR)
  {
    EnterCriticalSection(&outputCriticalSection);
    std::cerr << "Failed to connect to server with error: " << WSAGetLastError() << std::endl;
    LeaveCriticalSection(&outputCriticalSection);
    closesocket(clientSocket);
    WSACleanup();
    system("pause");
    exit(0);
  }
}

DWORD __stdcall Client::responsReceiver(const LPVOID lpvParam)
{
  Client &client = *((std::pair<Client*, size_t>*)lpvParam)->first;
  size_t requestNumber = ((std::pair<Client*, size_t>*)lpvParam)->second;
  delete lpvParam;

  for (size_t iteration = 0; iteration < requestNumber; ++iteration)
  {
    if (client.commandQueue.empty())
    {
      ResetEvent(client.commandSended);
      WaitForSingleObject(client.commandSended, INFINITE);
    }

    size_t responsLen;
    int result;
    result = recv(client.clientSocket, (char*)&responsLen, sizeof(responsLen), 0);
    if (result == 0)
    {
      EnterCriticalSection(&client.outputCriticalSection);
      std::cout << "Server disonnected." << std::endl;
      LeaveCriticalSection(&client.outputCriticalSection);
      break;
    }
    else if (result < 0)
    {
      EnterCriticalSection(&client.outputCriticalSection);
      std::cerr << "recv() failed with error: " << WSAGetLastError() << std::endl;
      LeaveCriticalSection(&client.outputCriticalSection);
      break;
    }

    wchar_t *command = new wchar_t[responsLen];
    result = recv(client.clientSocket, (char*)command, responsLen * sizeof(wchar_t), 0);
    if (result == 0)
    {
      EnterCriticalSection(&client.outputCriticalSection);
      std::cout << "Client disonnected." << std::endl;
      LeaveCriticalSection(&client.outputCriticalSection);
      break;
    }
    else if (result < 0)
    {
      EnterCriticalSection(&client.outputCriticalSection);
      std::cerr << "recv() failed with error: " << WSAGetLastError() << std::endl;
      LeaveCriticalSection(&client.outputCriticalSection);
      break;
    }

    EnterCriticalSection(&client.outputCriticalSection);
    std::wcout << command << std::endl;
    LeaveCriticalSection(&client.outputCriticalSection);
    delete[]command;
  }
  SetEvent(client.receiverTerminated);

  return 0;
}

Client::Client() : commands(), commandQueue()
{
  InitializeCriticalSection(&outputCriticalSection);
  readData();
  tuneNetwork();

  commandSended = CreateEventA(NULL, false, false, NULL);
  if (commandSended == INVALID_HANDLE_VALUE)
  {
    EnterCriticalSection(&outputCriticalSection);
    std::cerr << "Failed creating event!" << std::endl;
    LeaveCriticalSection(&outputCriticalSection);
    system("pause");
    exit(0);
  }
  receiverTerminated = CreateEventA(NULL, false, false, NULL);
  if (receiverTerminated == NULL)
  {
    EnterCriticalSection(&outputCriticalSection);
    std::cerr << "Failed creating event!" << GetLastError() << std::endl;
    LeaveCriticalSection(&outputCriticalSection);
    system("pause");
    exit(0);
  }

}

Client::~Client()
{
  DeleteCriticalSection(&outputCriticalSection);
  CloseHandle(receiverTerminated);
  CloseHandle(commandSended);
  closesocket(clientSocket);
  WSACleanup();
}

void Client::runClient(size_t requestNumber)
{
  auto receiverParam = new std::pair<Client*, size_t>(this, requestNumber);
  HANDLE clientHandlerReceiverThread = CreateThread(NULL, 0, responsReceiver, receiverParam, 0, NULL);

  for (size_t iteration = 0; iteration < requestNumber; ++iteration)
  {
    auto command = commands[iteration % commands.size()];

    int result;
    size_t len = command.size() + 1;
    result = send(clientSocket, (char*)&len, sizeof(len), 0);
    if (result == 0)
    {
      EnterCriticalSection(&outputCriticalSection);
      std::cout << "Server disconnected." << std::endl;
      LeaveCriticalSection(&outputCriticalSection);
      break;
    }
    else if (result < 0)
    {
      EnterCriticalSection(&outputCriticalSection);
      std::cerr << "send() failed with error: " << WSAGetLastError() << std::endl;
      LeaveCriticalSection(&outputCriticalSection);
      break;
    }
    
    result = send(clientSocket, (char*)command.c_str(), sizeof(wchar_t) * len, 0);
    if (result == 0)
    {
      EnterCriticalSection(&outputCriticalSection);
      std::cout << "Server disconnected." << std::endl;
      LeaveCriticalSection(&outputCriticalSection);
      break;
    }
    else if (result < 0)
    {
      EnterCriticalSection(&outputCriticalSection);
      std::cerr << "send() failed with error: " << WSAGetLastError() << std::endl;
      LeaveCriticalSection(&outputCriticalSection);
      break;
    }

    commandQueue.push(command);
    SetEvent(commandSended);

    if (iteration + 1 != requestNumber) Sleep(sleepTime);
  }
  WaitForSingleObject(receiverTerminated, INFINITE);

}
