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

    char host[NI_MAXHOST];
    char service[NI_MAXSERV];
  };
  const std::string dataFileName = "commandResponse.xml";
  const size_t maxConnections = 10;
  const uint32_t port = 8731;
  
  std::queue<std::pair<std::wstring, int>> commandQueue;
  std::vector<HandlerInfo> handlerInfo;
  std::queue<size_t> freeSocket;
  HANDLE commandPushed;
  HANDLE clientDisconnected;
  CRITICAL_SECTION outputCriticalSection;

  SOCKET listenSocket;

  void readData(std::unordered_map<std::wstring, std::wstring>&);

  void tuneNetwork();

  static DWORD __stdcall listeningSocket(const LPVOID lpvParam);
  static DWORD __stdcall clientHandlerReceiver(const LPVOID lpvParam);
  static DWORD __stdcall clientHandlerSender(const LPVOID lpvParam);

public:
  Server();
  ~Server();

  void runServer();

};

