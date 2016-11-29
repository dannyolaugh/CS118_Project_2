#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>

#include "helper.h"

int main(int argc, char** argv)
{
  struct sockaddr_in addr;
  struct sockaddr_in remaddr;
  socklen_t addrlen = sizeof(remaddr);
  int recvlen;
  char buffer[1032]; 

  if(argc != 2)
    {
      cerr << "ERROR: Invalid number of arguments";
    }

  string urlOrIP = argv[1];
  string portNum = argv[2]; 
  string ip;
  if(!isIP(urlOrIP))
    ip = urlOrIP;
  else
    ip = getIP(urlOrIP, portNum);
  
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    perror("cannot create socket\n");
    return 0;
  }
  
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    return 1;
  }

  // bind address to socket                                                         
  addr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(80);
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  if (::bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind");
    return 2;
  }

  memset((char *) &remaddr, 0, sizeof(remaddr));
  remaddr.sin_family = AF_INET;
  remaddr.sin_port = htons(portNum.c_str());
  if (inet_aton(ip, &remaddr.sin_addr)==0) {
    fprintf(stderr, "inet_aton() failed\n");
    exit(1);
  }

  /* now loop, receiving data and printing what we received */
  for (;;)
    {
      recvlen = recvfrom(sockfd, buffer, 1032, 0, 
			 (struct sockaddr *)&remaddr, &addrlen);
      if (recvlen > 0) 
	{
	  
	}
      else if (recvlen == 0)
	{

	}
      else
	{
	  fprintf(stderr, "recvfrom() failed");
	  exit(1);
	}
    }
}
