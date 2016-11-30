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
#include <cstring>

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
  payloadSize = 0;
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

string TCPmessage::getPayload()
{return payload;}

void TCPmessage::setPayload(string p)
{payload = p;}

void TCPmessage::setPayloadSize(int s)
{payloadSize = s;}

int TCPmessage::getPayloadSize()
{return payloadSize;}

char* TCPmessage::encode()
{
  char* e = new char[1032];

  memcpy(e, &sequenceNum, 2);
  memcpy(e+2, &ackNum, 2);
  memcpy(e+4, &cwnd, 2);

  int flags = 1*F + 2*S + 4*A;
  memcpy(e+6, &flags, 2);
  memcpy(e+8, payload.c_str(), payload.length());  
  
  return e;
}

void TCPmessage::decode(char* message)
{
  memcpy(&sequenceNum, message, 2);
  memcpy(&ackNum, message+2, 2);
  memcpy(&cwnd, message+4, 2);
  
  F = 0;
  S = 0;
  A = 0;
  

  int tmp = 0;
  memcpy(&tmp, message+6, 2);
  

  if(tmp%2 == 1)
    {
      F = 1;
    }
  if(tmp%4 > 1) 
    {
      S = 1;
    }
  if(tmp > 3)
    {
      A = 1;
    }
  
  char* tmpString = new char[1024];
  memcpy(tmpString, message+8, payloadSize);

  payload = tmpString;
}


bool isIP(string x)
{
  for(int i = 0; i < x.length(); i ++)
    {
      if(x[i] !=  '.' || !isdigit(x[i]))
	return false;
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


