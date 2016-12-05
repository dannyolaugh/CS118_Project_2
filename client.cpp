#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/time.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include "helper.h"

int main(int argc, char** argv)
{
  struct sockaddr_in addr;
  struct sockaddr_in remaddr;
  socklen_t addrlength = sizeof(remaddr);
  int recvlen;
  char buffer[1032]; 

  if(argc != 3)
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

  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 500000;
  setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO,(const char *)&tv,
             sizeof(struct timeval));

  // bind address to socket                                                         
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(0);
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  if (::bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind");
    return 2;
  }

  memset((char *) &remaddr, 0, sizeof(remaddr));
  remaddr.sin_family = AF_INET;
  remaddr.sin_port = htons(atoi(portNum.c_str()));
  if (inet_aton(ip.c_str(), &remaddr.sin_addr)==0) {
    fprintf(stderr, "inet_aton() failed\n");
    exit(1);
  }

  vector<TCPmessage> savedPackets;
  TCPmessage SYN(0, 0, 1024, 0, 1, 0);
  cout<< "Sending packet " << SYN.getackNum() << " SYN" << endl;
  if (sendto(sockfd, SYN.encode(), 8, 0,
	     (struct sockaddr *)&remaddr, addrlength) == -1)
    {
      perror("sendto error");
      return 3;
    } 
  SYN.setTimer();
  savedPackets.push_back(SYN);

  vector<TCPmessage> rec;
  int lastAck = 0;
  bool retrans = false;
  /* now loop, receiving data and printing what we received */
  for (;;)
    {
      recvlen = recvfrom(sockfd, buffer, 1032, 0, 
			 (struct sockaddr *)&remaddr, &addrlength);

      if (recvlen > 0) 
	{
	  TCPmessage recPacket(0,0,0,0,0,0);
	  recPacket.setPayloadSize(recvlen-8);
	  recPacket.decode(buffer);

	  if(lastAck > recPacket.getSequence())
	    continue;


	  cout << "Receiving packet " << recPacket.getSequence()
	    + recPacket.getPayloadSize() << endl;

	  if(recPacket.getF() == 1 && recPacket.getA() == 0)
	    {
	      vector<TCPmessage>::iterator it = savedPackets.begin();
              for(; it != savedPackets.end(); ++it)
                {
                  if((it->getackNum()) == recPacket.getSequence()){
                    savedPackets.erase(it);
                    break;
                  }
                }

	      
	      TCPmessage FIN(recPacket.getackNum(), recPacket.getSequence()+1,
			     recPacket.getcwnd(),0,0,1);

	      FIN.setTimer();
	      cout<< "Sending packet " << FIN.getackNum() << " FIN" << endl;
	      if (sendto(sockfd, FIN.encode(),
			 8, 0, (struct sockaddr *)&remaddr,
			 addrlength) == -1)
		{
		  perror("sendto error");
		  return 3;
		}

	      savedPackets.push_back(FIN);
	      TCPmessage FIN_ACK(recPacket.getackNum(), recPacket.getSequence()+1,
				 recPacket.getcwnd(),1,0,1);
	      
	      lastAck++;
	      savedPackets.push_back(FIN_ACK);
	      FIN_ACK.setTimer();
	      if (sendto(sockfd, FIN_ACK.encode(),8, 0,
			 (struct sockaddr *)&remaddr, addrlength) == -1)
		{
		  perror("sendto error");
		  return 3;
		}
	    }
	  else if(recPacket.getA() == 1)
	    {
	      if(recPacket.getS() == 1)
		{
		  savedPackets.clear();

		  TCPmessage SYN_ACK(recPacket.getackNum(), 
				     recPacket.getSequence() + 1, 
				     recPacket.getcwnd(), 1, 1, 0);


		  lastAck=1;
		  SYN_ACK.setTimer();
		  cout << "Sending packet " << SYN_ACK.getackNum() << " SYN" <<endl;
		  if (sendto(sockfd, SYN_ACK.encode(), 8, 0,
			     (struct sockaddr *)&remaddr, addrlength) == -1)
		    {
		      perror("sendto error");
		      return 3;
		    }
		  savedPackets.push_back(SYN_ACK);
		}
	      else
		{
		  break;
		}
	    }
	  else if(recPacket.getA() == 0 && 
		  recPacket.getF() == 0 && 
		  recPacket.getS() == 0)
	    {
	      rec.push_back(recPacket);
	      int nextAck = recPacket.getSequence() + recPacket.getPayloadSize();

	      vector<TCPmessage>::iterator it = savedPackets.begin();
              for(; it != savedPackets.end(); ++it) 
		{
		  if((it->getackNum()) == recPacket.getSequence()){
		    savedPackets.erase(it);
		    break;
		  }
		}
	      
	      if(retrans)
		{
		  if(nextAck <= lastAck + recPacket.getPayloadSize())
		    {
		      retrans = false;
		      vector<TCPmessage>::iterator it = rec.begin();
		      int largest = it->getSequence();
		      for(; it < rec.end(); it++)
			if(it->getSequence() < (it+1)->getSequence())
			  largest = (it+1)->getSequence();
		      nextAck = largest;
		      lastAck = nextAck;
		    }
		}

	      bool tmp =false;
	      if(nextAck > lastAck + recPacket.getPayloadSize())
	      	{
		  retrans = true;
		  nextAck = lastAck;
		}
	      else
	      	tmp = true;
	      
	      if(recPacket.getSequence()+ recPacket.getPayloadSize() > 30720)
		nextAck -= 30720;
	      
	      if(tmp)
	      	lastAck = nextAck;
	      
	      TCPmessage ACK(recPacket.getackNum(), nextAck,
			     recPacket.getcwnd(),1,0,0);

	      ACK.setTimer();
	      cout<< "Sending packet " << ACK.getackNum() << endl;
	      savedPackets.push_back(ACK);
	      if (sendto(sockfd, ACK.encode(),
			 8, 0, (struct sockaddr *)&remaddr,
			 addrlength) == -1)
		{
		  perror("sendto error");
		  return 3;
		}	      
	    }
	  else
	    {
	      cout << "wrong flags" << endl;
	      return 12;
	    }
	}
      else if (recvlen == 0)
	{
	  cout <<  "connection closed";
	}
      else
	{

	}
      bool timeout = false;
      timeval timestamp;
      gettimeofday(&timestamp, NULL);
      vector<TCPmessage>::iterator it = savedPackets.begin();
      long int timeDiffSec = timestamp.tv_sec - it->getTimeSet().tv_sec;
      long int timeDiffUSec = timestamp.tv_usec - it->getTimeSet().tv_usec;
      if(timeDiffSec > 0)
	timeout = true;
      else if(timeDiffUSec > 500000)
	timeout = true;

      if(timeout)
	{
	  it->setTimer();

	  cout<< "Sending packet " << it->getackNum() << " Retransmission";
	  if(it->getS())
	    cout << " SYN";
	  if(it->getF())
	    cout << " FIN";

	  cout << endl;
	  if (sendto(sockfd, it->encode(),
		     8, 0, (struct sockaddr *)&remaddr,
		     addrlength) == -1)
	    {
	      perror("sendto error");
	      return 3;
	    }
	  if(it->getF())
	    {
	      if (sendto(sockfd, (it+1)->encode(),
			 8, 0, (struct sockaddr *)&remaddr,
			 addrlength) == -1)
		{
		  perror("sendto error");
		  return 3;
		}
	    }
	  
	  continue;
	}
    }

  
  bool swapped = true;
  while (swapped) 
    {
      swapped = false;
      vector<TCPmessage>::iterator it = rec.begin();
      for (; it != rec.end()-1; it++) 
	{
	  if (it->getSequence() > (it+1)->getSequence())
	    {
	      TCPmessage tmp(it->getSequence(), 0,0,0,0,0);
	      tmp.setPayload(it->getPayload());
	      it->setSequence((it+1)->getSequence());
	      it->setPayload((it+1)->getPayload());
	      (it+1)->setSequence(tmp.getSequence());
	      (it+1)->setPayload(tmp.getPayload());
	      swapped = true;
	    }
	}
    }

  string file = "";


  vector<TCPmessage>::iterator it = rec.begin();
  for(; it < rec.end(); it++)
    if(it->getSequence() != (it+1)->getSequence())
      file += it->getPayload();

  
  ofstream outf;
  outf.open("received.data");
  if (outf.is_open()) {
    outf << file;
  }
  outf.close();

  close(sockfd);
}
