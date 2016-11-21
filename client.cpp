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

  /* bind the socket to any valid IP address and a specific port */
  struct sockaddr_in serverAddr;
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(atoi(portNum.c_str()));     // short, network byte order
  serverAddr.sin_addr.s_addr = inet_addr(ip.c_str());
  memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

  struct sockaddr_in clientAddr;
  socklen_t clientAddrLen = sizeof(clientAddr);
  if (getsockname(sockfd, (struct sockaddr *)&clientAddr, &clientAddrLen) == -1) {
    perror("getsockname");
    return 3;
  }



  /* now loop, receiving data and printing what we received */
  for (;;) {
    printf("waiting on port %d\n", PORT);
    recvlen = recvfrom(fd, buf, BUFSIZE, 0, (struct sockaddr *)&remaddr, &addrlen);
    printf("received %d bytes\n", recvlen);
    if (recvlen > 0) {
      buf[recvlen] = 0;
      printf("received message: \"%s\"\n", buf);
    }
  }
}
