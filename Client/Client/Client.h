#pragma once
#include"Headers.h"

class Client
{
  const std::string commandsFileName = "commands.xml";
  const std::string serverIpAddress = "127.0.0.1";
  const uint32_t port = 8731;

  size_t requestNumber;

  SOCKET clientSocket;

  std::vector<std::wstring> commands;

  void readData();
  void tuneNetwork();

public:

  Client(size_t requestNumber);
  ~Client();

};

