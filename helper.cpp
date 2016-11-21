#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ctype.h>
#include <string>
#include <iostream>

#include "helper.h"

using namespace std;

TCPmessage::TCPmessage(int sNum, int aNum, int c, int aFlag, int sFlag, int fFlag)
{
  sequenceNum = sNum;
  ackNum = aNum;
  cwnd = c;
  A = aFlag;
  S = sFlag;
  F = fFlag;
  payload = "";
}

int TCPmessage::getSequence()
{return sequenceNum;}

int TCPmessage::getackNum()
{return ackNum;}

int TCPmessage::getcwnd()
{return cwnd;}

int TCPmessage::getA()
{return A;}

int TCPmessage::getS()
{return S;}

int TCPmessage::getF()
{return F;}

void TCPmessage::setPayload(string p)
{
  payload = p;
}

char* TCPmessage::encode()
{
  char* e = new char[1032];

  memcpy(e, &sequenceNum, 2);
  memcpy(e+2, &ackNum, 2);
  memcpy(e+4, &cwnd, 2);

  char* tmp[2];
  tmp[0] = '0';
  int flags = 1*F + 2*S + 4*A;
  tmp[1] = flags + '0';
  memcpy(e+6, tmp, 2);
  
  memcpy(e+8, payload.c_str(), payload.length());  
  
}

void TCPmessage::decode(char* m)
{
  TCPmessage 
}

bool isIP(string x)
{
  for(int i = 0; i < x.length(); i ++)
    {
      if(x[i] != "." || !isdigit(x[i]))
	return false
    }
  return true;
}


string getIP(string hostname, string portNum)
{
  //get the ip address
  struct addrinfo hints;
  struct addrinfo* res;

  // prepare hints                                                          
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET; // IPv4                                        
  hints.ai_socktype = SOCK_STREAM; // TCP                                   

  // get address                                                            
  int status = 0;
  if ((status = getaddrinfo(hostname.c_str(), portNum.c_str(), &hints, &res)) != 0) {
    std::cerr << "getaddrinfo: " << gai_strerror(status) << std::endl;
  }

  string ip = "";
  for(struct addrinfo* p = res; p != 0; p = p->ai_next) {
    // convert address to IPv4 address                                      
    struct sockaddr_in* ipv4 = (struct sockaddr_in*)p->ai_addr;

    // convert the IP to a string and print it:                             
    char ipstr[INET_ADDRSTRLEN] = {'\0'};
    inet_ntop(p->ai_family, &(ipv4->sin_addr), ipstr, sizeof(ipstr));
    ip = ipstr;
  }
  freeaddrinfo(res); // free the linked list     
  return ip;
}

