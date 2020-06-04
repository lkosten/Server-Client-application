#pragma once

#include "Headers.h"

class Server
{
  struct HandlerInfo
  {
    std::queue<std::wstring> responseQueue;
    HANDLE responsePushed;
    size_t index;
    SOCKET clientSocket;
  };
  const std::string dataFileName = "commandResponse.xml";
  const size_t maxConnections = 10;
  const uint32_t port = 8731;
  
  std::queue<std::pair<std::wstring, int>> commandQueue;
  std::vector<HandlerInfo> handlerInfo;
  std::queue<size_t> freeSocket;
  HANDLE commandPushed;
  HANDLE clientDisconnected;

  SOCKET listenSocket;

  void readData(std::unordered_map<std::wstring, std::wstring>&);

  void tuneNetwork();

  static DWORD __stdcall listeningSocket(LPVOID lpvParam);
  static DWORD __stdcall clientHandler(LPVOID lpvParam);

public:
  Server();
  ~Server();

  void runServer();

};

