#ifndef HELPER_H
#define HELPER_H

#include <string>

using namespace std;

bool isIP(string x);
string getIP(string hostname, string portNum);

class TCPmessage
{
 public:
  TCPmessage(int sNum, int aNum, int c, int aFlag, int sFlag, int fFlag);
  int getSequence();
  int getackNum();
  int getcwnd();
  int getS();
  int getF();
  int getA();
  string getPayload();
  void setPayload(string p);
  void setPayloadSize(int s);
  int getPayloadSize();

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
};

#endif
