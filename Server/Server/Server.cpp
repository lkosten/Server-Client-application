#include "Server.h"

void Server::readData(std::unordered_map<std::wstring, std::wstring> &commandToResponse)
{
  std::wifstream inputFile;
  inputFile.open(dataFileName);
  if (!inputFile)
  {
    std::cerr << "Cann't open file with commands!";
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

Server::Server() : requestQueue()
{

}

Server::~Server()
{
}


void Server::runServer()
{
  std::unordered_map<std::wstring, std::wstring> commandToResponse;
  readData(commandToResponse);
  
}
