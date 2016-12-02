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

  // allow others to reuse the address
  int yes = 1;
  if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    return 1;
  }
  
  // bind address to socket
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(port_num.c_str()));     // short, network byte order
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  memset(addr.sin_zero, '\0', sizeof(addr.sin_zero));

  if (::bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind");
    return 2;
  }

  int x = 0;
  string file;
  unsigned char i;  
  ifstream myReadFile;
  myReadFile.open(file_name, ios::binary);
  if(myReadFile.fail())
    {
      perror("open");
      return 4;
    }

  bool done = false;
  int lastAckNum = -1;
  for (;;)
    {
      recvlen = recvfrom(sockfd, buffer, 1032, 0, 
			 (struct sockaddr *)&remaddr, &addrlength);
      if (recvlen > 0)
        { 
	  TCPmessage recPacket(0,0,0,0,0,0);
	  recPacket.decode(buffer);
	  
	  int nextAck = 0;
	  if(recPacket.getSequence() != 30720)
	    {
	      nextAck = recPacket.getSequence() + 1;
	    }
	  if(done == true)
	    {
	      TCPmessage FIN(recPacket.getackNum(), nextAck,
			     recPacket.getcwnd(),0,0,1);
	      
	      if (sendto(sockfd, FIN.encode(),
			 8, 0, (struct sockaddr *)&remaddr,
			 addrlength) == -1)
		{
		  perror("sendto error");
		  return 3;
		}
	      
	      done = false;
	    }
	  else if(recPacket.getF() == 1)
	    {
	      if(recPacket.getS() == 1)
		{
		  perror("SYN-FIN no allow");
		  return 7;
		}
	      if(recPacket.getA() == 1)
		{
		  cout << "fin ack received" << endl;
		}
              else
                {
		  cout << "fin received" << endl;
		  TCPmessage FIN_ACK(recPacket.getackNum(), nextAck,
				     recPacket.getcwnd(),1,0,1);
		  
		  if (sendto(sockfd, FIN_ACK.encode(),
			     8, 0, (struct sockaddr *)&remaddr,
			     addrlength) == -1)
		    {
		      perror("sendto error");
		      return 3;
		    }
		}
            }
	  else if(recPacket.getA() == 1)
	    {
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

              if(recPacket.getS() == 1)
		{
		  if(file.size() == 1024)
		    {
		      cout << "syn ack full received" <<endl;
		      TCPmessage initial_packet(recPacket.getackNum(), nextAck,
						recPacket.getcwnd(), 0, 0, 0);
		      
		      initial_packet.setPayload(file);
		      initial_packet.setPayloadSize(1024);
		      
		      if (sendto(sockfd, initial_packet.encode(), 
				 1032, 0, (struct sockaddr *)&remaddr,
				 addrlength) == -1)
			{
			  perror("sendto error");
			  return 3;
			}
		    }
		  else
		    {
		      cout << "syn ack only received" <<endl;
		      TCPmessage initial_packet(recPacket.getackNum(), nextAck,
						recPacket.getcwnd(), 0, 0, 0);

		      initial_packet.setPayload(file);
		      initial_packet.setPayloadSize(file.size());

                      if (sendto(sockfd, initial_packet.encode(),
                                 8 + file.size(),
                                 0, (struct sockaddr *)&remaddr,
                                 addrlength) == -1)
                        {
                          perror("sendto error");
                          return 3;
                        }
		      
		      done = true;

		      lastAckNum = recPacket.getackNum() + file.size();
		      if(recPacket.getackNum() + file.size() > 30720)
			lastAckNum -= 30720;
		    }
	 	}
	      else
		{
		  if(file.size() == 1024)
                    {
		      cout << "ack received" << endl;
		      TCPmessage packet(recPacket.getackNum(), nextAck,
					recPacket.getcwnd(), 0, 0, 0);
		      
                      packet.setPayload(file);
		      packet.setPayloadSize(1024);

                      if (sendto(sockfd, packet.encode(),
                                 1032, 0, (struct sockaddr *)&remaddr,
                                 addrlength) == -1)
                        {
                          perror("sendto error");
                          return 3;
                        }
                    }
                  else
                    {
		      cout << "ack only received" <<endl;
		      TCPmessage packet(recPacket.getackNum(), nextAck,
					recPacket.getcwnd(), 0, 0, 0);
		      
		      packet.setPayload(file);
		      packet.setPayloadSize(file.size());
		      
                      if (sendto(sockfd, packet.encode(),
                                 8 + file.size(),
                                 0, (struct sockaddr *)&remaddr,
                                 addrlength) == -1)
                        {
                          perror("sendto error");
                          return 3;
                        }
		      done = true;
		    }
		}
            }
	  else if(recPacket.getS() == 1)
	    {
	      cout << "syn received" <<endl;
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
	      perror("No flags set");
	      return 7;
	    }
	}
      else if (recvlen == 0)
	{

        }
      else
	{
          fprintf(stderr, "recvfrom() failed");
          exit(1);
        }
    }
  close(sockfd);
}
