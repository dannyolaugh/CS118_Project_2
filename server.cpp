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
#include <vector>
#include <sys/time.h>
#include <iostream>
#include <sstream>
#include <fstream>

using namespace std;

int main(int argc, char* argv[])
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
  string host_name = "localhost";
  string port_num = argv[1];
  string file_name = argv[2];

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
  addr.sin_port = htons(atoi(port_num.c_str()));     // short, network byte order
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  if (::bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind");
    return 2;
  }

  string file = "";
  int x = 0;
  unsigned char i;  
  ifstream myReadFile;
  myReadFile.open(file_name, ios::binary);
  if(myReadFile.fail())
    {
      perror("open");
      return 4;
    }

  int lastSequence = 0;
  int lastAck = -1;
  int numPacketsOut = 0;  
  int cwnd = 1024;
  bool done = false;
  int ssthresh = 15360;
  bool retransmit = false;
  vector<TCPmessage> savedPackets;
  bool firstTime = false;
  bool secondTime = false;
  bool finished = false;
  timeval f;
  int nextAck = 0;
  for (;;)
    {
      recvlen = recvfrom(sockfd, buffer, 1032, 0, 
			 (struct sockaddr *)&remaddr, &addrlength);

      if(finished)
	{
	  timeval tmp;
	  gettimeofday(&tmp, NULL);
	  if(tmp.tv_sec > f.tv_sec)
	    {
	      savedPackets.clear();
	      finished = false;
	      cwnd = 1024;
	      ssthresh = 15360;
	      numPacketsOut = 0;
	      done = false;
	      lastSequence = 0;
	      retransmit = false;
	      firstTime = false;
	      secondTime = false;
	      lastAck = -1;
	      f = (struct timeval){0};
	      myReadFile.close();
	      myReadFile.open(file_name, ios::binary);
	      if(myReadFile.fail())
		{
		  perror("open");
		  return 4;
		}
	      continue;
	    }
	}

      if(done == true && numPacketsOut == 0)
	{
	  TCPmessage FIN(lastAck, lastSequence, cwnd, 0, 0, 1);

	  FIN.setTimer();
	  cout<< "Sending packet " << FIN.getSequence() << " "
	      << FIN.getcwnd() << " " << ssthresh << " FIN" <<endl;
	  if (sendto(sockfd, FIN.encode(),
		     8, 0, (struct sockaddr *)&remaddr,
		     addrlength) == -1)
	    {
	      perror("sendto error");
	      return 3;
	    }
	  numPacketsOut = 1;
	  done = true;
	  savedPackets.push_back(FIN);
	}
      

      if (recvlen > 0)
        { 
	  TCPmessage recPacket(0,0,0,0,0,0);
	  recPacket.decode(buffer);

	  if(secondTime && recPacket.getackNum() == lastAck){
	    retransmit= true;

	  }
	  if(firstTime && recPacket.getackNum() == lastAck){
            secondTime = true;

	  }
	  if(recPacket.getackNum() == lastAck){
	    firstTime = true;
	  }
	    

	  
	  if(retransmit)
            {
              ssthresh = cwnd/2;
              if(ssthresh < 1024)
                ssthresh = 1024;
              cwnd = 1024;
	      
	      vector<TCPmessage>::iterator it = savedPackets.begin();
	      for(; it < savedPackets.end(); it++)
		if(it->getSequence() == lastAck)
		  break;
	      
	      it->setTimer();	      
	      
	      cout<< "Sending packet " << it->getSequence() << " "
                  << it->getcwnd() << " " << ssthresh << " Retransmission";
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
	      retransmit = false;
	      firstTime = false;
	      secondTime = false;
	      it->setTimer();
	      continue;
	    }
	  
	  lastAck = recPacket.getackNum();

	  if(recPacket.getS())
	    cwnd =1024;
	  	  
	 if(recPacket.getF() == 1)
	    {
	      if(recPacket.getS() == 1)
		{
		  perror("SYN-FIN no allow");
		  return 7;
		}
	      if(recPacket.getA() == 1)
		{
		  numPacketsOut--;
		  cout<< "Receiving packet " << recPacket.getackNum() << endl;
		  vector<TCPmessage>::iterator it = savedPackets.begin();
		  for(; it != savedPackets.end(); ++it) {
		    if((it->getSequence()) == recPacket.getackNum()-1){
		      savedPackets.erase(it);
		      break;
		    }
		  }
		}
              else
                {
		  savedPackets.clear();
		  
		  TCPmessage FIN_ACK(recPacket.getackNum(), 
				     recPacket.getSequence()+1,
                                     recPacket.getcwnd(),1,0,1);

                  savedPackets.push_back(FIN_ACK);
		  FIN_ACK.setTimer();
		  if (sendto(sockfd, FIN_ACK.encode(),8, 0,
                             (struct sockaddr *)&remaddr, addrlength) == -1)
                    {
                      perror("sendto error");
                      return 3;
                    }
		  finished = true;
		  numPacketsOut++;
		  gettimeofday(&f, NULL);
		}
            }
	  else if(recPacket.getA() == 1)
	    {
	      numPacketsOut--;
	      cout<< "Receiving packet " << recPacket.getackNum() << endl;

	      vector<TCPmessage>::iterator it = savedPackets.begin();
	      for(; it != savedPackets.end(); ++it) {
		if((it->getSequence()) == recPacket.getackNum() - 1024){
		  savedPackets.erase(it);
		  break;
		}
		if(it->getSequence() == recPacket.getackNum() - 1)
		  {
		    savedPackets.erase(it);
		    break;
		  }
		if(it->getSequence() < recPacket.getackNum() && done
		   &&it->getSequence() - recPacket.getackNum() < 1024)
		  {
		    savedPackets.erase(it);
		    break;
		  }
		if((it->getSequence() - 30720) == (recPacket.getackNum() - 1024))
		  {
		    savedPackets.erase(it);
		    break;
		  }
	      }

	      lastAck = recPacket.getackNum();

	      for(int n = numPacketsOut; n < cwnd/1024; n++)
		{
		  nextAck = recPacket.getackNum() + (numPacketsOut*1024);
		  if(nextAck > 30720)
		    {
		      nextAck -= 30720;
		    }
		  lastAck = nextAck;

		  if(done == true)
		    break;
		  file = "";
		  x = 0;
		  while(x < 1024) 
		    {
		      myReadFile.read((char *)&i, sizeof(i));
		      if(myReadFile.eof())
			break;
		      file += i;
		      x++;
		    }
		  if(file.size() == 0)
		    {
		      numPacketsOut--;
		      break;
		    }

		  if(file.size() == 1024)
		    {
		      TCPmessage packet(nextAck,
					recPacket.getSequence(), cwnd,0,0,0);
		      		      
		      packet.setPayload(file);
		      packet.setPayloadSize(1024);
		      
		      cout << "Sending packet " << packet.getSequence() << " "
			   << packet.getcwnd() << " " << ssthresh <<  endl;
		      
		      packet.setTimer();
		      if (sendto(sockfd, packet.encode(),
				 1032, 0, (struct sockaddr *)&remaddr,
				 addrlength) == -1)
			{
			  perror("sendto error");
			  return 3;
			}
		      savedPackets.push_back(packet);
		    }
		  else
		    {
		      TCPmessage packet(nextAck,
					recPacket.getSequence(),cwnd,0,0,0);
		      lastSequence = recPacket.getSequence();
		      
		      packet.setPayload(file);
		      packet.setPayloadSize(file.size());
		      
		      cout << "Sending packet " << packet.getSequence() << " "
			   << packet.getcwnd() << " " << ssthresh <<   endl;
		      
		      packet.setTimer();
		      if (sendto(sockfd, packet.encode(),
				 8 + file.size(),
				 0, (struct sockaddr *)&remaddr,
				 addrlength) == -1)
			{
			  perror("sendto error");
			  return 3;
			}
		      done = true;
		      savedPackets.push_back(packet);
		    }
		  numPacketsOut++;		  
		}
	    }
	  else if(recPacket.getS() == 1)
	    {
	      cout<< "Receiving packet " << recPacket.getackNum() << endl;
	      TCPmessage SYN_ACK(recPacket.getackNum(),
				 recPacket.getSequence()+1,cwnd,1,1,0);

	      cout << "Sending packet " << SYN_ACK.getSequence() << " "
		   << SYN_ACK.getcwnd() << " " << ssthresh << " SYN" <<endl ;

	      lastAck = recPacket.getackNum();
	      SYN_ACK.setTimer();
	      if (sendto(sockfd, SYN_ACK.encode(), 8, 0, 
			 (struct sockaddr *)&remaddr, addrlength) == -1)
		{
		  perror("sendto error");
		  return 3;
		}
	      numPacketsOut++;
	      savedPackets.push_back(SYN_ACK);
	    }
	  else
	    {
	      perror("No flags set");
	      return 7;
	    }
	 if(cwnd> 28762)
	   continue;

	 if(cwnd > ssthresh)
	   cwnd+=1024;
	 else
	   cwnd *= 2;

	   }
      else if (recvlen == 0)
	{

        }
      else
	{

        }


      if(savedPackets.size()==0)
	{
	  continue;
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
	  ssthresh = cwnd/2;
	  if(ssthresh < 1024)
	    ssthresh = 1024;
	  cwnd = 1024;

	  it->setTimer();

	  cout<< "Sending packet " << it->getSequence() << " "
	      << it->getcwnd() << " " << ssthresh << " Retransmission";
	  if(it->getS())
	    cout << " SYN";
	  if(it->getF())
	    {
	      done = true;
	      cout << " FIN";
	    }
	  cout << endl;

	  if (sendto(sockfd, it->encode(),
		     8 + it->getPayloadSize(), 0, (struct sockaddr *)&remaddr,
		     addrlength) == -1)
	    {
	      perror("sendto error");
	      return 3;
	    }
	  retransmit = false;
	  firstTime = false;
	  secondTime = false;
	  continue;
	}
    }
  close(sockfd);
}
