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
  
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    return 1;
  }

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

  TCPmessage SYN(0, 0, 1024, 0, 1, 0);

  if (sendto(sockfd, SYN.encode(), 8, 0,
	     (struct sockaddr *)&remaddr, addrlength) == -1)
    {
      perror("sendto error");
      return 3;
    }
  cout << "syn sent" <<endl;
  string file = ""; 

  /* now loop, receiving data and printing what we received */
  for (;;)
    {
      recvlen = recvfrom(sockfd, buffer, 1032, 0, 
			 (struct sockaddr *)&remaddr, &addrlength);
      cout <<recvlen<< endl;
      TCPmessage recPacket(0,0,0,0,0,0);
      recPacket.setPayloadSize(recvlen-8);
      cout << recPacket.getPayloadSize() << endl;
      recPacket.decode(buffer);

      if (recvlen > 0) 
	{
	  if(recPacket.getF() == 1 && recPacket.getA() == 0)
	    {
	      int nextAck = 0;
	      if(recPacket.getSequence() != 30720)
		nextAck = recPacket.getSequence() + 1;
	      
	      cout << "fin received" <<endl;
	      TCPmessage FIN(recPacket.getackNum(), nextAck,
			     recPacket.getcwnd(),0,0,1);

	      if (sendto(sockfd, FIN.encode(),
			 8, 0, (struct sockaddr *)&remaddr,
			 addrlength) == -1)
		{
		  perror("sendto error");
		  return 3;
		}


	      TCPmessage FIN_ACK(recPacket.getackNum(), nextAck,
				 recPacket.getcwnd(),1,0,1);
	      
	      if (sendto(sockfd, FIN_ACK.encode(),8, 0,
			 (struct sockaddr *)&remaddr, addrlength) == -1)
		{
		  perror("sendto error");
		  return 3;
		}
	      break;
	    }
	  else if(recPacket.getA() == 1)
	    {
	      if(recPacket.getS() == 1)
		{
		  int nextAck = 0;
		  if(recPacket.getSequence() != 30720)
		    nextAck= recPacket.getSequence() + 1;

		  cout << "syn ack received" <<endl;
		  TCPmessage SYN_ACK(recPacket.getackNum(), nextAck, 
				     recPacket.getcwnd(), 1, 1, 0);
		  
		  if (sendto(sockfd, SYN_ACK.encode(), 8, 0,
			     (struct sockaddr *)&remaddr, addrlength) == -1)
		    {
		      perror("sendto error");
		      return 3;
		    }
		}
	      else
		{
		  cout << "server Ack'd == no bueno" << endl;
		  return 11;
		}
	    }
	  else if(recPacket.getA() == 0 && 
		  recPacket.getF() == 0 && 
		  recPacket.getS() == 0)
	    {
	      cout << "data received" <<endl;
	      file += recPacket.getPayload();

	      cout << recPacket.getPayload().size() << endl;
	      int nextAck = recPacket.getSequence() + recPacket.getPayloadSize();
	      if(recPacket.getSequence()+ recPacket.getPayloadSize() > 30720)
		nextAck -= 30720;

	      TCPmessage ACK(recPacket.getackNum(), nextAck,
			     recPacket.getcwnd(),1,0,0);
	      
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
	  fprintf(stderr, "recvfrom() failed");
	  exit(1);
	}
    }

  ofstream outf;
  outf.open("requested_data.txt");
  if (outf.is_open()) {
    outf << file;
  }
  outf.close();
  close(sockfd);
}
