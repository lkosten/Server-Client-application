#pragma once
#include"Headers.h"

class Client
{
  const std::string commandsFileName = "commands.xml";
  const std::string serverIpAddress = "127.0.0.1";
  const uint32_t port = 8731;

  std::vector<std::wstring> commands;
  std::queue<std::wstring> commandQueue;

  HANDLE commandSended;

  SOCKET clientSocket;

  void readData();
  void tuneNetwork();

  static DWORD __stdcall responsReceiver(const LPVOID lpvParam);

public:

  Client();
  ~Client();

  void runClient(size_t requestNumber);

};

