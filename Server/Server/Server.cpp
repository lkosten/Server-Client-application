#include "Server.h"

void Server::readData(std::unordered_map<std::wstring, std::wstring> &commandToResponse)
{
  std::wifstream inputFile;
  inputFile.open(dataFileName);
  if (!inputFile)
  {
    std::cerr << "Cann't open file with commands!" << std::endl;
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

Server::Server() : commandQueue(), handlerInfo(maxConnections)
{
  commandPushed = CreateEvent(NULL, false, false, NULL);
  if (commandPushed == NULL)
  {
    std::cerr << "Failed creating event!" << std::endl;
    exit(0);
  }

  for (size_t i = 0; i < maxConnections; ++i)
  {
    handlerInfo[i].index = i;
    handlerInfo[i].responsePushed = CreateEvent(NULL, false, false, NULL);
    if (handlerInfo[i].responsePushed == NULL)
    {
      std::cerr << "Failed creating commandPushed event!" << std::endl;
      exit(0);
    }
  }
}

Server::~Server()
{
  CloseHandle(commandPushed);
  for (auto &element : handlerInfo) CloseHandle(element.responsePushed);
}


void Server::runServer()
{
  std::unordered_map<std::wstring, std::wstring> commandToResponse;
  readData(commandToResponse);
  

  HANDLE listeningSocketThread = CreateThread(NULL, 0, this->listeningSocket, NULL, 0, NULL);
  if (listeningSocketThread == NULL)
  {
    std::cerr << "Failed creating listeningSocket thread!" << std::endl;
    exit(0);
  }
  else CloseHandle(listeningSocketThread);

  while (true)
  {
    if (commandQueue.empty()) WaitForSingleObject(commandPushed, INFINITE);

    auto curCommand = commandQueue.front();
    commandQueue.pop();

    handlerInfo[curCommand.second].responseQueue.push(commandToResponse[curCommand.first]);
    SetEvent(handlerInfo[curCommand.second].responsePushed);
  }
}

void __stdcall Server::listeningSocket()
{

}
