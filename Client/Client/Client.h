#pragma once
#include"Headers.h"

class Client
{
  const std::string commandsFileName = "commands.xml";
  const std::string serverIpAddress = "127.0.0.1";
  const uint32_t port = 8731;
  const DWORD sleepTime = 1000;
  std::vector<std::wstring> commands;
  std::queue<std::wstring> commandQueue;

  HANDLE commandSended;
  HANDLE receiverTerminated;
  CRITICAL_SECTION outputCriticalSection;

  SOCKET clientSocket;

  void readData();
  void tuneNetwork();

  static DWORD __stdcall responsReceiver(const LPVOID lpvParam);
  void runClient(size_t requestNumber);

public:

  Client(size_t requestNumber);
  ~Client();


};

