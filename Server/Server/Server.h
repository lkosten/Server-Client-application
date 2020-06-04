#pragma once

#include "Headers.h"

class Server
{
  struct HandlerInfo
  {
    std::queue<std::wstring> responseQueue;
    HANDLE responsePushed;
    size_t index;
  };
  const std::string dataFileName = "commandResponse.xml";
  const size_t maxConnections = 10;
  const uint32_t port = 8731;
  
  std::queue<std::pair<std::wstring, int>> commandQueue;
  std::vector<HandlerInfo> handlerInfo;
  HANDLE commandPushed;

  SOCKET listenSocket;

  void readData(std::unordered_map<std::wstring, std::wstring>&);

  void tuneNetwork();

  static DWORD __stdcall listeningSocket(const LPVOID lpvParam);
public:
  Server();
  ~Server();

  void runServer();

};

