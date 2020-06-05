#pragma once
#include"Headers.h"

class Client
{
  const std::string commandsFileName = "commands.xml";
  size_t requestNumber;

  std::vector<std::wstring> 

  void readData();

public:

  Client(size_t requestNumber);
  ~Client();

};

