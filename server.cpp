#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <thread>
#include "helper.h"

#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;


int main(int argc, char* argv[])
{
  if(argc != 2)
    {
      cerr << "ERROR: Invalid number of arguments";
    }
  string host_name = "localhost";
  string port_num = argv[1];
  string file_name = argv[2];

  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("cannot create socket\n");
    return 0;
  }

  // allow others to reuse the address
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    return 1;
  }
  
  // bind address to socket
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(port_number.c_str()));     // short, network byte order
  addr.sin_addr.s_addr = inet_addr(getIP(host_name, port_number).c_str());
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  if (::bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind");
    return 2;
  }

}
