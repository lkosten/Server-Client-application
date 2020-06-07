#include "Client.h"

int main(int argc, char **argv)
{
  _setmode(_fileno(stdout), _O_U16TEXT);
  Client client(INFINITE);
  
  system("pause");
  return 0;
}