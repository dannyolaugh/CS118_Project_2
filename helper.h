#ifndef HELPER_H
#define HELPER_H

#include <string>
#include <sys/time.h>
using namespace std;

bool isIP(string x);
string getIP(string hostname, string portNum);

class TCPmessage
{
 public:
  TCPmessage(int sNum, int aNum, int c, int aFlag, int sFlag, int fFlag);
  int getSequence();
  void setSequence(int s);
  int getackNum();
  int getcwnd();
  int getS();
  int getF();
  int getA();
  string getPayload();
  void setPayload(string p);
  void setPayloadSize(int s);
  int getPayloadSize();
  timeval getTimeSet();
  void setTimer();

  char* encode();
  void decode(char* message);

 private:
  int sequenceNum;
  int ackNum;
  int cwnd;
  int A;
  int S;
  int F;
  int payloadSize;
  string payload;
  timeval sentTime;
};

#endif
