#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <math.h>	
#include <time.h>
#include "listener.h"	

int packet_number=0;

#define INPUT_PORT_SPINNAKER 44444 //port to receive spinnaker packets

#pragma pack(1) //stop alignment in structure: word alignment would be nasty here, byte alignment reqd


struct sdp_msg {
	unsigned short  reserved;
	unsigned char   flags;
	unsigned char   ip_tag;
	unsigned char   destination_port;
	unsigned char   source_port;
	unsigned short  destination_chip;
	unsigned short  source_chip;
	unsigned short  command;
	unsigned short  sequence;
	unsigned int    arg1;
	unsigned int    arg2;
	unsigned int    arg3;
	unsigned char data;  }; 	// a structure that holds SpiNNaker packet data (inside UDP segment)


//global variables for spinnaker packet receiver
int sockfd_input;
char portno_input[6];
struct addrinfo hints_input, *servinfo_input, *p_input;
struct sockaddr_storage their_addr_input;
socklen_t addr_len_input;
int rv_input;
int numbytes_input;
struct sdp_msg * scanptr;
unsigned char buffer_input[1515];  //buffer for network packets
//end of variables for spinnaker packet receiver - some could be local really - but with pthread they may need to be more visible


// prototypes for functions below
void error(char *msg);
void init_udp_server_spinnaker();
void* input_thread (void *ptr);
// end of prototypes

// setup socket for SpiNNaker frame receiving on port 54321 (or per define as above)
void init_udp_server_spinnaker()
{
	snprintf (portno_input, 6, "%d", INPUT_PORT_SPINNAKER);

	bzero(&hints_input, sizeof(hints_input));
	hints_input.ai_family = AF_INET; // set to AF_INET to force IPv4
	hints_input.ai_socktype = SOCK_DGRAM; // type UDP (socket datagram)
	hints_input.ai_flags = AI_PASSIVE; // use my IP

	if ((rv_input = getaddrinfo(NULL, portno_input, &hints_input, &servinfo_input)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv_input));
		exit(1);
	}

	// loop through all the results and bind to the first we can
	for(p_input = servinfo_input; p_input != NULL; p_input = p_input->ai_next) {
		if ((sockfd_input = socket(p_input->ai_family, p_input->ai_socktype, p_input->ai_protocol)) == -1) {
			perror("SpiNNaker listener: socket");
			continue;
		}

		if (bind(sockfd_input, p_input->ai_addr, p_input->ai_addrlen) == -1) {
			close(sockfd_input);
			perror("SpiNNaker listener: bind");
			continue;
		}

		break;
	} 

	if (p_input == NULL) {
		fprintf(stderr, "SpiNNaker listener: failed to bind socket\n");
		exit(-1);
	}

	freeaddrinfo(servinfo_input);

	printf ("Spinnaker UDP listener setup complete!\n");  	// here ends the UDP listener setup witchcraft
}

void* input_thread (void *ptr)
{
	printf("\n\nListening for SpiNNaker frames - you'll see nothing if there are none!\n\n\n");
	while (1) { 						// for ever ever, ever ever.
		int numAdditionalBytes = 0;

		addr_len_input = sizeof their_addr_input;
		if ((numbytes_input = recvfrom(sockfd_input, buffer_input, 1514 , 0, (struct sockaddr *)&their_addr_input, &addr_len_input)) == -1) {
			perror((char*)"error recvfrom");
			exit(-1);		// will only get here if there's an error getting the input frame off the Ethernet
		}

        scanptr = (struct sdp_msg*) buffer_input;

        char *data = (char*)(&(scanptr->data));

        if ((int)(scanptr->command) == 3)
        {
            lastMessageTime = clock();

            int numberOfPixels = (int)(scanptr->arg1);

            int i;
            for (i=0; i<numberOfPixels; i++)
            {
                unsigned char x1 = ((unsigned char)data[i*7]);
                unsigned char x2 = ((unsigned char)data[i*7+1]);
                unsigned char y1 = ((unsigned char)data[i*7+2]);
                unsigned char y2 = ((unsigned char)data[i*7+3]);
                int x = (((int)x1)<<8) + ((int)x2);
                int y = (((int)y1)<<8) + ((int)y2);
                unsigned char r = ((unsigned char)data[i*7+4]);
                unsigned char g = ((unsigned char)data[i*7+5]);
                unsigned char b = ((unsigned char)data[i*7+6]);

                if (frameBuffering)
                {
                    drawingFrame[((frameHeight-y-1)*(frameWidth)+x)*3]   = r;
                    drawingFrame[((frameHeight-y-1)*(frameWidth)+x)*3+1] = g;
                    drawingFrame[((frameHeight-y-1)*(frameWidth)+x)*3+2] = b;
                }
                else
                {
                    viewingFrame[((frameHeight-y-1)*(frameWidth)+x)*3]   = r;
                    viewingFrame[((frameHeight-y-1)*(frameWidth)+x)*3+1] = g;
                    viewingFrame[((frameHeight-y-1)*(frameWidth)+x)*3+2] = b;
                }

            }
        }

        packet_number++;
		
		fflush (stdout);    // flush IO buffers now - and why not? (immortal B. Norman esq)
	}
}


void send_trace_trigger(int camx, int camy, int camz, int lookx, int looky, int lookz, int upx, int upy, int upz, int width, int height, int hField, int vField, int antialiasing, int x, int y, int processor, int globalNodeID, int numberOfNodes)
{
    
}


void error(char *msg)
{
	perror(msg);
	exit(1);
}   
