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

  if(argc != 2)
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

  string file;
  unsigned char i;  
  ifstream myReadFile;
  myReadFile.open(file_name, ios::binary);
  if(myReadFile.fail())
    {
      perror("open");
      return 4;
    }
  //read file if found                                                          
  while(1) {
    myReadFile.read((char *)&i, sizeof(i));
    if(myReadFile.eof())
      break;
    file += i;
  }


  for (;;)
    {
      recvlen = recvfrom(sockfd, buffer, 1032, 0, 
			 (struct sockaddr *)&remaddr, &addrlength);
      if (recvlen > 0)
        { 
	  TCPmessage recPacket(0,0,0,0,0,0);
	  recPacket.decode(buffer);

	  if(recPacket.getF() == 1)
	    {
	      if(recPacket.getS() == 1)
		{
		  perror("SYN-FIN no allow");
		  return 7;
		}
              if(recPacket.getA() == 1)
		{
		  
                }
              else
                {
		  
                }
            }
	  else if(recPacket.getA() == 1)
	    {
              if(recPacket.getS() == 1)
		{
		  if(file.size() > 1024)
		    {
		      TCPmessage initial_packet(recPacket.getackNum(),
						recPacket.getSequence() + 1024,
						recPacket.getcwnd(), 1, 1, 0);
		      
		      initial_packet.setPayload(file.substr(0,1024));
		      
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
		      TCPmessage initial_packet(recPacket.getackNum(),
				       recPacket.getSequence() + file.size(),
				       recPacket.getcwnd(), 1, 1, 0);
		      
		      initial_packet.setPayload(file.substr(0,file.size()));

                      if (sendto(sockfd, initial_packet.encode(),
                                 8 + file.size(),
                                 0, (struct sockaddr *)&remaddr,
                                 addrlength) == -1)
                        {
                          perror("sendto error");
                          return 3;
                        }
		    }
		}
	      else
		{
		  if(file.size() > 1024 + recPacket.getackNum())
                    {
		      TCPmessage packet(recPacket.getackNum(),
				       1024 + recPacket.getackNum(),
				       recPacket.getcwnd(), 1, 0, 0);
		      
                      packet.setPayload(file.substr(recPacket.getackNum(),1024));

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
		      TCPmessage packet(recPacket.getackNum(),
				       file.size() - recPacket.getackNum(),
				       recPacket.getcwnd(), 1, 0, 0);

		      packet.setPayload(file.substr(recPacket.getackNum(),
							    file.size() -
							    recPacket.getackNum()));

                      if (sendto(sockfd, packet.encode(),
                                 file.size() - recPacket.getackNum(),
                                 0, (struct sockaddr *)&remaddr,
                                 addrlength) == -1)
                        {
                          perror("sendto error");
                          return 3;
                        }
                    }

		}
            }
	  else if(recPacket.getS() == 1)
	    {
	      TCPmessage SYN_ACK(recPacket.getackNum(),0,recPacket.getcwnd(), 1, 1, 0);
	      
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
}
