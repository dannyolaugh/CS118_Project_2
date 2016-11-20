#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <string>
#include <iostream>

#include "helper.h"

using namespace std;

void HttpRequest::parseUrl(string url)
{
  //make request message based off URl
  method = "GET";
  version = "HTTP/1.0";
  port = "80";

  int pos = 0;
  bool portDef = false;
  bool fileNameFind = false;
  for(unsigned int i = 7; i < url.length(); i++)
    {
      if(url[i] == ':')
	{
	  host = url.substr(7,i-7);
	  pos = i;
	  portDef = true;
	}
      else if(url[i] == '/' && portDef && !fileNameFind)
        {
	  port = url.substr(pos+1, i-pos-1);
	  path = url.substr(i, url.length()-1);
	  fileNameFind = true;
        }
      else if(url[i] == '/' && !portDef && !fileNameFind)
	{
	  host = url.substr(7,i-7);
	  path = url.substr(i, url.length()-1);
	  fileNameFind = true;
	}
      if(url[i] == '/')
	{
	  pos = i;
	}
    }
  fileName = url.substr(pos + 1, url.length()-1);
}


void HttpRequest::setMethod(string m)
{
  method = m;
}

string HttpRequest::getMethod()
{
  return method;
}

void HttpRequest::setVersion(string v)
{
  version = v;
}

string HttpRequest::getVersion()
{
  return version;
}

void HttpRequest::setPath(string p)
{
  path = p;
}

string HttpRequest::getPath()
{
  return path;
}

void HttpRequest::setPort(string p)
{
  port = p;
}

string HttpRequest::getPort()
{
  return port;
}

void HttpRequest::setHost(string h)
{
  host = h;
}

string HttpRequest::getHost()
{
  return host;
}

void HttpRequest::setFileName(string f)
{
  fileName = f;
}

string HttpRequest::getFileName()
{
  return fileName;
}

string HttpRequest::encode()
{
  //encode the message
  message = method + " " + path + " " + version + "\r\n" 
    + "Host: " + host + ":" + port + "\r\n\r\n";
  return message;
}

void HttpRequest::decode(string message)
{
  //decode an Http Request
  int count = 0;
  int pos = 0;
  for(unsigned int i = 0; i < message.length(); i++)
    {
      if(message[i] == ' ' && count == 0)
	{
	  method = message.substr(0,i);
	  count++;
	  pos = i;
	}
      else if(message[i] == ' ' && count == 1)
	{
	  path = message.substr(pos+1, i-pos-1);
	  count++;
	  pos = i;
	}
      else if (message[i] == '\r' && count == 2)
	{
	  version = message.substr(pos+1, i-pos);
	  pos = i;
	  count++;
	}
    }
}

bool HttpRequest::isValid()
{
  //check if valid request
  if(method != "GET" || version == "")
    return false;
  return true;
}

/*////////////////////////////////*/
/*///////////BREAK////////////////*/
/*////////////////////////////////*/

void HttpResponse::setResponse(string s, string b)
{
  version = "HTTP/1.0";
  status = s;
  body = b;
  bodySize = body.size();
  message = "";
}


void HttpResponse::setVersion(string v)
{
  version = v;
}

string HttpResponse::getVersion()
{
  return version;
}

void HttpResponse::setStatus(string s)
{
  status = s;
}

string HttpResponse::getStatus()
{
  return status;
}

void HttpResponse::setBody(string b)
{
  body = b;
}

string HttpResponse::getBody()
{
  return body;
}

void HttpResponse::setBodySize(int b)
{
  bodySize = b;
}

int HttpResponse::getBodySize()
{
  return bodySize;
}


string HttpResponse::encode()
{
  message = version + " " + status + "\r\n" + "Content-Length: " 
    + to_string(bodySize) + "\r\n\r\n" + body;

  return message;
}

void HttpResponse::decode(string message)
{
  //decode response message
  version = message.substr(0,8);
  int i = message.find("\r\n");
  status = message.substr(9, i-9);
  message.erase(0,i+2);
  i = message.find("\r\n\r\n");
  body = message.substr(i+3,message.length()-i-3);
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

