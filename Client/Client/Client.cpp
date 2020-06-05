#include "Client.h"


void Client::readData()
{
  std::wifstream inputFile;
  inputFile.open(commandsFileName);
  if (!inputFile)
  {
    std::cerr << "Can't open file with commands!" << std::endl;
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

Client::Client(size_t requestNumber) : requestNumber(requestNumber)
{

}

Client::~Client()
{
}
