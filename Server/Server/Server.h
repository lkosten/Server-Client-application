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

  std::queue<std::pair<std::wstring, int>> commandQueue;
  std::vector<HandlerInfo> handlerInfo;
  HANDLE commandPushed;

  void readData(std::unordered_map<std::wstring, std::wstring>&);

public:
  Server();
  ~Server();

  void runServer();

  void __stdcall listeningSocket();
};

