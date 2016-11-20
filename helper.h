#ifndef HELPER_H
#define HELPER_H

#include <string>

using namespace std;

class HttpRequest 
{
public:
  void parseUrl(string url);
  bool isValid();   
  void setMethod(string m);
  string getMethod();
  void setVersion(string v);
  string getVersion();
  void setPort(string p);
  string getPort();
  void setPath(string p);
  string getPath();
  void setHost(string h);
  string getHost();
  void setMessage(string m);
  string getMessage();
  void setFileName(string f);
  string getFileName();
  string encode();
  void decode(string message);

private:
  string method;
  string version;
  string port;
  string path;
  string message;
  string host;
  string fileName;
};

class HttpResponse
{
public:
  void setResponse(string s, string b);
  
  void setVersion(string v);
  string getVersion();
  void setStatus(string s);
  string getStatus();
  void setBody(string b);
  string getBody();
  void setBodySize(int b);
  int getBodySize();
  void setMessage(string m);
  string getMessage();
  string encode();
  void decode(string message);

private:
  string version;
  string status;
  string body;
  string message;
  int bodySize;
};


string getIP(string hostname, string portNum);


#endif
