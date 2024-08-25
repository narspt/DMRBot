/*
    XRFBot - ChatGPT Voice Bot for DStar
    Copyright (C) 2024 Nuno Silva

    Based on code from https://github.com/nostar/reflector_connectors
    Copyright (C) 2019 Doug McLain

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <stdio.h> 
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h> 
#include <string.h> 
#include <netdb.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>
#include <sys/ioctl.h>

#define AMBE_ENCODE_GAIN -5
#define AMBE_DECODE_GAIN 5
#define CLIENT_MOD 'B'
//#define XRF1_DXRFD_COMPAT
#define BUFSIZE 2048
#define TIMEOUT 60
//#define DEBUG_SEND
//#define DEBUG_RECV

char 		*ref1;
struct sockaddr_in 	host1;
struct sockaddr_in 	host2;
int 				udp1;
int 				udp2;
fd_set 				udpset; 
uint8_t 			buf[BUFSIZE];
char 				callsign[8U];

const uint16_t CCITT16_TABLE1[] = {
	0x0000U, 0x1189U, 0x2312U, 0x329bU, 0x4624U, 0x57adU, 0x6536U, 0x74bfU,
	0x8c48U, 0x9dc1U, 0xaf5aU, 0xbed3U, 0xca6cU, 0xdbe5U, 0xe97eU, 0xf8f7U,
	0x1081U, 0x0108U, 0x3393U, 0x221aU, 0x56a5U, 0x472cU, 0x75b7U, 0x643eU,
	0x9cc9U, 0x8d40U, 0xbfdbU, 0xae52U, 0xdaedU, 0xcb64U, 0xf9ffU, 0xe876U,
	0x2102U, 0x308bU, 0x0210U, 0x1399U, 0x6726U, 0x76afU, 0x4434U, 0x55bdU,
	0xad4aU, 0xbcc3U, 0x8e58U, 0x9fd1U, 0xeb6eU, 0xfae7U, 0xc87cU, 0xd9f5U,
	0x3183U, 0x200aU, 0x1291U, 0x0318U, 0x77a7U, 0x662eU, 0x54b5U, 0x453cU,
	0xbdcbU, 0xac42U, 0x9ed9U, 0x8f50U, 0xfbefU, 0xea66U, 0xd8fdU, 0xc974U,
	0x4204U, 0x538dU, 0x6116U, 0x709fU, 0x0420U, 0x15a9U, 0x2732U, 0x36bbU,
	0xce4cU, 0xdfc5U, 0xed5eU, 0xfcd7U, 0x8868U, 0x99e1U, 0xab7aU, 0xbaf3U,
	0x5285U, 0x430cU, 0x7197U, 0x601eU, 0x14a1U, 0x0528U, 0x37b3U, 0x263aU,
	0xdecdU, 0xcf44U, 0xfddfU, 0xec56U, 0x98e9U, 0x8960U, 0xbbfbU, 0xaa72U,
	0x6306U, 0x728fU, 0x4014U, 0x519dU, 0x2522U, 0x34abU, 0x0630U, 0x17b9U,
	0xef4eU, 0xfec7U, 0xcc5cU, 0xddd5U, 0xa96aU, 0xb8e3U, 0x8a78U, 0x9bf1U,
	0x7387U, 0x620eU, 0x5095U, 0x411cU, 0x35a3U, 0x242aU, 0x16b1U, 0x0738U,
	0xffcfU, 0xee46U, 0xdcddU, 0xcd54U, 0xb9ebU, 0xa862U, 0x9af9U, 0x8b70U,
	0x8408U, 0x9581U, 0xa71aU, 0xb693U, 0xc22cU, 0xd3a5U, 0xe13eU, 0xf0b7U,
	0x0840U, 0x19c9U, 0x2b52U, 0x3adbU, 0x4e64U, 0x5fedU, 0x6d76U, 0x7cffU,
	0x9489U, 0x8500U, 0xb79bU, 0xa612U, 0xd2adU, 0xc324U, 0xf1bfU, 0xe036U,
	0x18c1U, 0x0948U, 0x3bd3U, 0x2a5aU, 0x5ee5U, 0x4f6cU, 0x7df7U, 0x6c7eU,
	0xa50aU, 0xb483U, 0x8618U, 0x9791U, 0xe32eU, 0xf2a7U, 0xc03cU, 0xd1b5U,
	0x2942U, 0x38cbU, 0x0a50U, 0x1bd9U, 0x6f66U, 0x7eefU, 0x4c74U, 0x5dfdU,
	0xb58bU, 0xa402U, 0x9699U, 0x8710U, 0xf3afU, 0xe226U, 0xd0bdU, 0xc134U,
	0x39c3U, 0x284aU, 0x1ad1U, 0x0b58U, 0x7fe7U, 0x6e6eU, 0x5cf5U, 0x4d7cU,
	0xc60cU, 0xd785U, 0xe51eU, 0xf497U, 0x8028U, 0x91a1U, 0xa33aU, 0xb2b3U,
	0x4a44U, 0x5bcdU, 0x6956U, 0x78dfU, 0x0c60U, 0x1de9U, 0x2f72U, 0x3efbU,
	0xd68dU, 0xc704U, 0xf59fU, 0xe416U, 0x90a9U, 0x8120U, 0xb3bbU, 0xa232U,
	0x5ac5U, 0x4b4cU, 0x79d7U, 0x685eU, 0x1ce1U, 0x0d68U, 0x3ff3U, 0x2e7aU,
	0xe70eU, 0xf687U, 0xc41cU, 0xd595U, 0xa12aU, 0xb0a3U, 0x8238U, 0x93b1U,
	0x6b46U, 0x7acfU, 0x4854U, 0x59ddU, 0x2d62U, 0x3cebU, 0x0e70U, 0x1ff9U,
	0xf78fU, 0xe606U, 0xd49dU, 0xc514U, 0xb1abU, 0xa022U, 0x92b9U, 0x8330U,
	0x7bc7U, 0x6a4eU, 0x58d5U, 0x495cU, 0x3de3U, 0x2c6aU, 0x1ef1U, 0x0f78U };


int max(int x, int y) 
{ 
    if (x > y) 
        return x; 
    else
        return y; 
} 

void process_signal(int sig)
{
	uint8_t b[20];
	if(sig == SIGINT){
		fprintf(stderr, "\n\nShutting down link\n");
		memcpy(b, callsign, 8);
		b[8] = CLIENT_MOD;
		b[9] = ' ';
		b[10] = 0x00;
		sendto(udp1, b, 11, 0, (const struct sockaddr *)&host1, sizeof(host1));
#ifdef DEBUG_SEND
		fprintf(stderr, "SEND %s: ", ref1);
		for(int i = 0; i < 11; ++i)
			fprintf(stderr, "%02x ", b[i]);
		fprintf(stderr, "\n");
#endif
		close(udp1);
		close(udp2);
		exit(EXIT_SUCCESS);
	}
	if(sig == SIGALRM){
		memcpy(b, callsign, 8);
		b[8] = 0x00;
		sendto(udp1, b, 9, 0, (const struct sockaddr *)&host1, sizeof(host1));
#ifdef DEBUG_SEND
		fprintf(stderr, "SEND %s: ", ref1);
		for(int i = 0; i < 9; ++i)
			fprintf(stderr, "%02x ", b[i]);
		fprintf(stderr, "\n");
#endif
		alarm(5);
	}
}

typedef struct wav_header_t {
  // RIFF Header
  char riff_header[4]; // Contains "RIFF"
  int wav_size; // Size of the wav portion of the file, which follows the first 8 bytes. File size - 8
  char wave_header[4]; // Contains "WAVE"
  // Format Header
  char fmt_header[4]; // Contains "fmt " (includes trailing space)
  int fmt_chunk_size; // Should be 16 for PCM
  short audio_format; // Should be 1 for PCM. 3 for IEEE Float
  short num_channels; // Number of channels
  int sample_rate; // Sample rate (hz)
  int byte_rate; // Number of bytes per second. sample_rate * num_channels * Bytes Per Sample
  short sample_alignment; // Number of bytes per sample. num_channels * Bytes Per Sample
  short bit_depth; // Number of bits per sample
  // Data
  char data_header[4]; // Contains "data"
  int data_bytes; // Number of bytes in data. Number of samples * num_channels * sample byte size
} wav_header;

void addCCITT161(unsigned char *in, unsigned int length)
{
	
	union C{
		uint16_t crc16;
		uint8_t  crc8[2U];
	} c;


	c.crc16 = 0xFFFFU;

	for (unsigned int i = 0U; i < (length - 2U); i++)
		c.crc16 = (c.crc8[1U]) ^ CCITT16_TABLE1[c.crc8[0U] ^ in[i]];

	c.crc16 = ~(c.crc16);

	in[length - 2U] = c.crc8[0U];
	in[length - 1U] = c.crc8[1U];
}

int main(int argc, char **argv)
{
	struct 	sockaddr_in rx;
	struct 	hostent *hp;
	struct 	timeval tv;
	char *mod1;
	char *	host1_url;
	char *	host2_url;
	int 	host1_port;
	int 	host2_port;
	int 	rxlen;
	int 	r;
	int 	udprx,maxudp;
	socklen_t l = sizeof(host1);
	time_t pong_time1;
	const uint8_t header[4] = {0x44,0x53,0x56,0x54}; 	//packet header
	int32_t rx_streamid = -1;
	uint16_t tx_streamid = 0;
	time_t rx_endt = 0;
	FILE *rx_ambefile = NULL;
	FILE *rx_wavefile = NULL;
	FILE *tx_wavefile = NULL;
	wav_header rx_wavheader;
	int rx_ambefcnt = 0;
	int tx_ambefcnt = 0;
	int64_t trgus = 0;
	char rx_callsign[8U];
	bool txpending = false;
	
	//change stdout/stderr to line buffering
	setvbuf(stdout, NULL, _IOLBF, 0);
	setvbuf(stderr, NULL, _IOLBF, 0);
	
	//change working directory to the executable directory
	char exepath[4096] = {0};
	if (readlink("/proc/self/exe", exepath, sizeof(exepath)-1) == -1) {
		fprintf(stderr, "failed to get executable path\n");
		return 1;
	}
	char *exelastslash = strrchr(exepath, '/');
	if (exelastslash == NULL) {
		fprintf(stderr, "failed to extract executable directory\n");
		return 1;
	}
	*(exelastslash+1) = '\0';
	if (chdir(exepath) == -1) {
		fprintf(stderr, "failed to change working directory\n");
		return 1;
	}
	
	//seed random generator
	srand(time(NULL));
	
	if(argc != 4){
		fprintf(stderr, "Usage: xrfbot [CALLSIGN] [XRFName:MOD:XRFHostIP:PORT] [AMBEServerIP:PORT]\n");
		return 0;
	}
	else{
		memset(callsign, ' ', 8);
		memcpy(callsign, argv[1], (strlen(argv[1])<8)?strlen(argv[1]):8);
		
		ref1 = strtok(argv[2], ":");
		mod1 = strtok(NULL, ":");
		host1_url = strtok(NULL, ":");
		host1_port = atoi(strtok(NULL, ":"));
		host2_url = strtok(argv[3], ":");
		host2_port = atoi(strtok(NULL, ":"));
		
		printf("XRF: %s%c %s:%d\n", ref1, mod1[0], host1_url, host1_port);
		printf("AMBEServer: %s:%d\n", host2_url, host2_port);
	}
	
	signal(SIGINT, process_signal); 						//Handle CTRL-C gracefully
	signal(SIGALRM, process_signal); 						//Ping timer
	
	if ((udp1 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		return 0;
	}
	
	if ((udp2 = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("cannot create socket");
		return 0;
	}

#ifdef XRF1_DXRFD_COMPAT
	memset((char *)&host1, 0, sizeof(host1));
	host1.sin_family = AF_INET;
	host1.sin_port = htons(30001);
	host1.sin_addr.s_addr = htonl(INADDR_ANY);
	if ( bind(udp1, (struct sockaddr *)&host1, sizeof(host1)) == -1 ) {
		fprintf(stderr, "error while binding the socket on port 30001\n");
		return 0;
	}
#endif
	
	maxudp = max(udp1, udp2) + 1;
	memset((char *)&host1, 0, sizeof(host1));
	host1.sin_family = AF_INET;
	host1.sin_port = htons(host1_port);
	memset((char *)&host2, 0, sizeof(host2));
	host2.sin_family = AF_INET;
	host2.sin_port = htons(host2_port);

	hp = gethostbyname(host1_url);
	if (!hp) {
		fprintf(stderr, "could not resolve %s\n", host1_url);
		return 0;
	}
	memcpy((void *)&host1.sin_addr, hp->h_addr_list[0], hp->h_length);

	hp = gethostbyname(host2_url);
	if (!hp) {
		fprintf(stderr, "could not resolve %s\n", host2_url);
		return 0;
	}
	memcpy((void *)&host2.sin_addr, hp->h_addr_list[0], hp->h_length);

	uint8_t host1_connect = 1;

	alarm(5);
	
	while (1) {
		if(host1_connect){
			host1_connect = 0;
			pong_time1 = time(NULL);
			memcpy(buf, callsign, 8);
			buf[8] = CLIENT_MOD;
			buf[9] = mod1[0];
			buf[10] = 0x00;
			sendto(udp1, buf, 11, 0, (const struct sockaddr *)&host1, sizeof(host1));
			fprintf(stderr, "Connecting to %s...\n", ref1);
#ifdef DEBUG_SEND
			fprintf(stderr, "SEND %s: ", ref1);
			for(int i = 0; i < 11; ++i)
				fprintf(stderr, "%02x ", buf[i]);
			fprintf(stderr, "\n");
#endif
		}
		FD_ZERO(&udpset);
		FD_SET(udp1, &udpset);
		FD_SET(udp2, &udpset);
		tv.tv_sec = 0;
		tv.tv_usec = 5*1000;
		r = select(maxudp, &udpset, NULL, NULL, &tv);
		//fprintf(stderr, "Select returned r == %d\n", r);
		rxlen = 0;
		if(r > 0){
			if(FD_ISSET(udp1, &udpset)) {
				rxlen = recvfrom(udp1, buf, BUFSIZE, 0, (struct sockaddr *)&rx, &l);
				udprx = udp1;
			}
			else if(FD_ISSET(udp2, &udpset)) {
				rxlen = recvfrom(udp2, buf, BUFSIZE, 0, (struct sockaddr *)&rx, &l);
				udprx = udp2;
			}
		}
#ifdef DEBUG_RECV
		if(rxlen){
			if ((udprx == udp1) && (rx.sin_addr.s_addr == host1.sin_addr.s_addr)){
				fprintf(stderr, "RECV %s: ", ref1);
			}
			else if((udprx == udp2) && (rx.sin_addr.s_addr == host2.sin_addr.s_addr)){
				fprintf(stderr, "RECV AMBE: ");
			}
			for(int i = 0; i < rxlen; ++i){
				fprintf(stderr, "%02x ", buf[i]);
			}
			fprintf(stderr, "\n");
		}
#endif
    if( rxlen && (udprx == udp1) && (rx.sin_addr.s_addr == host1.sin_addr.s_addr) ){
      if(rxlen == 9){ //keep-alive
        pong_time1 = time(NULL);
      }

      else if((rxlen == 56) && (!memcmp(&buf[0], header, 4))) { //dv header
        if( !memcmp(&buf[18], ref1, 6) && (buf[25] == mod1[0]) ){
          uint16_t s = (buf[12] << 8) | (buf[13] & 0xff);
          if(s != rx_streamid){
            rx_streamid = s;
            memcpy(rx_callsign, &buf[42], 8);

            /*if (rx_ambefile != NULL) fclose(rx_ambefile);
            rx_ambefile = fopen("rx.ambe" , "wb");
            static const uint8_t header[] = {'A','M','B','E'};
            if (rx_ambefile != NULL)
              fwrite(header, 1, sizeof(header), rx_ambefile);*/

            if (rx_wavefile != NULL) {
              rewind(rx_wavefile);
              rx_wavheader.data_bytes = rx_ambefcnt * 320;
              rx_wavheader.wav_size = rx_wavheader.data_bytes + sizeof(rx_wavheader) - 8;
              fwrite(&rx_wavheader, 1, sizeof(rx_wavheader), rx_wavefile);
              fclose(rx_wavefile);
              rx_wavefile = NULL;
            }

            memcpy(rx_wavheader.riff_header, "RIFF", 4);
            memcpy(rx_wavheader.wave_header, "WAVE", 4);
            memcpy(rx_wavheader.fmt_header, "fmt ", 4);
            rx_wavheader.fmt_chunk_size = 16;
            rx_wavheader.audio_format = 1;
            memcpy(rx_wavheader.data_header, "data", 4);
            rx_wavheader.num_channels = 1;
            rx_wavheader.sample_rate = 8000;
            rx_wavheader.bit_depth = 16;
            rx_wavheader.sample_alignment = (rx_wavheader.bit_depth / 8) * rx_wavheader.num_channels;
            rx_wavheader.byte_rate = rx_wavheader.sample_rate * rx_wavheader.sample_alignment;
            rx_wavheader.data_bytes = 0; //filled later
            rx_wavheader.wav_size = rx_wavheader.data_bytes + sizeof(rx_wavheader) - 8;

            rx_wavefile = fopen("rx.wav" , "wb");
            if (rx_wavefile != NULL) {
              fwrite(&rx_wavheader, 1, sizeof(rx_wavheader), rx_wavefile);
            } else {
              fprintf(stderr, "failed to open rx.wav file\n");
            }

            static const uint8_t ambe_gain[] = {0x61,0x00,0x03,0x00,0x4B,AMBE_ENCODE_GAIN,AMBE_DECODE_GAIN};
            sendto(udp2, ambe_gain, sizeof(ambe_gain), 0, (const struct sockaddr *)&host2, sizeof(host2));

            static const uint8_t ambe_ratep[] = {0x61,0x00,0x0D,0x00,0x0A,0x01,0x30,0x07,0x63,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x48};
            sendto(udp2, ambe_ratep, sizeof(ambe_ratep), 0, (const struct sockaddr *)&host2, sizeof(host2));
            
            printf("*** RX START (callsign: %.8s) ***\n", rx_callsign);
            rx_ambefcnt = 0;
            rx_endt = time(NULL)+2; //allow rx end without terminator, after extra timeout
          }
        }
      }

      else if(rxlen == 27){ //dv frame
        uint16_t s = (buf[12] << 8) | (buf[13] & 0xff);
        if(s == rx_streamid){
          if (rx_ambefile != NULL)
            fwrite(&buf[15], 1, 9, rx_ambefile);

          //send ambe frames to ambeserver
          uint8_t ambebuf[4+2+9] = {0x61, 0x00, 2+9, 0x01,  0x01, 72};
          memcpy(&ambebuf[6], &buf[15], 9);
          sendto(udp2, ambebuf, sizeof(ambebuf), 0, (const struct sockaddr *)&host2, sizeof(host2));
          
          rx_endt = time(NULL)+2; //allow rx end without terminator, after extra timeout
          
          if ((buf[14] & 0x40) != 0){ //last frame
            rx_streamid = -1;
            rx_endt = time(NULL)+1;
          }
        }
      }

    }

    else if( rxlen && (udprx == udp2) && (rx.sin_addr.s_addr == host2.sin_addr.s_addr) ){ //from ambeserver
      if ((rxlen == 4+2+320) && (buf[0] == 0x61) && (buf[3] == 0x02)) {
        if (rx_wavefile == NULL) { //if rx file not open, discard packet
#ifdef DEBUG_RECV
          fprintf(stderr, "*** discarding pcm packet from ambeserver ***\n");
#endif
          continue;
        }
        for (int i=0; i < 160; i++) //swap byte order for all samples, AMBE3000 uses MSB first
          ((unsigned short *)(&buf[6]))[i] = (((unsigned short *)(&buf[6]))[i] >> 8) | (((unsigned short *)(&buf[6]))[i] << 8);
        fwrite(&buf[6], 1, 320, rx_wavefile);
        rx_ambefcnt++;
      }
      else if ((rxlen == 4+2+9) && (buf[0] == 0x61) && (buf[3] == 0x01)) {
        if (tx_wavefile == NULL) { //if tx terminated, discard late packet from ambeserver, not good to tx them after terminator
#ifdef DEBUG_RECV
          fprintf(stderr, "*** discarding ambe packet from ambeserver ***\n");
#endif
          continue;
        }
        uint8_t tx_ambefr[9];
        memcpy(tx_ambefr, &buf[6], 9);

        memset(buf, 0, 27);
        memcpy(buf, header, 4);
        buf[4] = 0x20; //voice frame
        buf[8] = 0x20; //voice stream
        buf[9] = 0x00;
        buf[10] = 0x01;
        buf[11] = 0x01;
        buf[12] = (tx_streamid >> 8) & 0xFF; //stream id
        buf[13] = tx_streamid & 0xFF; //stream id
        buf[14] = tx_ambefcnt % 21; //frame counter
        memcpy(&buf[15], tx_ambefr, 9);

        if (buf[14] == 0) { //sync
          buf[24] = 0x55;
          buf[25] = 0x2D;
          buf[26] = 0x16;
        } else { //slow data
          buf[24] = 0x66 ^ 0x70;
          buf[25] = 0x66 ^ 0x4f;
          buf[26] = 0x66 ^ 0x93;
        }

        sendto(udp1, buf, 27, 0, (const struct sockaddr *)&host1, sizeof(host1));
#ifdef DEBUG_SEND
        fprintf(stderr, "SEND %s: ", ref1);
        for(int i = 0; i < 27; ++i)
          fprintf(stderr, "%02x ", buf[i]);
        fprintf(stderr, "\n");
#endif
        tx_ambefcnt++;
      }
    }

    //if ((host1_connect_status == CONNECTED_RW) && !rx_endt) { rx_endt=time(NULL)+5; } //dbg

    if (rx_endt && (time(NULL) > rx_endt)) { //rx end
        if (rx_ambefile != NULL) {
          fclose(rx_ambefile);
          rx_ambefile = NULL;
        }
        if (rx_wavefile != NULL) {
          rewind(rx_wavefile);
          rx_wavheader.data_bytes = rx_ambefcnt * 320;
          rx_wavheader.wav_size = rx_wavheader.data_bytes + sizeof(rx_wavheader) - 8;
          fwrite(&rx_wavheader, 1, sizeof(rx_wavheader), rx_wavefile);
          fclose(rx_wavefile);
          rx_wavefile = NULL;
          printf("*** RX END (ambeframes: %d) ***\n", rx_ambefcnt);
          rx_streamid = -1;
          
          if (txpending) { //if there is a pending tx, ignore current rx
            rx_endt = time(NULL)+1; //wait a bit more before starting the pending tx
            continue;
          }
          //if (rx_ambefcnt < 50) { //if we got less than 1 sec. of audio
          if (rx_ambefcnt < 1) { //if we got no audio frames
            rx_endt = 0; //cancel processing this short rx
            continue;
          }
          if (tx_wavefile != NULL) {
            fclose(tx_wavefile); //ensure tx wav file is closed before script tries to write on it
            tx_wavefile = NULL;
          }
          char cmdstr[50];
          sprintf(cmdstr, "python3 -u dmrbot.py %.8s", rx_callsign);
          if (system(cmdstr) != 0) {
            //rx_endt = 0; //cancel tx if script fails
            //continue;
            fprintf(stderr, "dmrbot.py returned error, tx unavailable.wav file...\n");
            system("cat unavailable.wav > tx.wav");
          }
          pong_time1 = time(NULL); //prevent timeout due to time spent on system() call
          rx_endt = time(NULL)+1; //wait a bit more before starting tx, allow rx to check if someone else tx
          txpending = true;
          continue;
        }
          
        //tx playback
        if ( txpending && (tx_wavefile == NULL) ) {
          printf("*** TX START ***\n");
          txpending = false;
          tx_ambefcnt = 0;
          trgus = 0;
          tx_streamid = (rand() % 0xffff) + 1;
          tx_wavefile = fopen("tx.wav" , "rb");
          if (tx_wavefile == NULL) {
            fprintf(stderr, "failed to open tx.wav file\n");
            rx_endt = 0; //cancel tx
            continue;
          }
          
          wav_header tx_wavheader;
          if ( fread(&tx_wavheader, 1, sizeof(tx_wavheader), tx_wavefile) != sizeof(tx_wavheader) ) {
            fprintf(stderr, "invalid wav file\n");
            fclose(tx_wavefile);
            tx_wavefile = NULL;
            rx_endt = 0; //cancel tx
            continue;
          }
          if (memcmp(tx_wavheader.data_header, "LIST", 4U) == 0) { //skip LIST chunk
            fseek(tx_wavefile, tx_wavheader.data_bytes, SEEK_CUR);
            fread(tx_wavheader.data_header, 1, sizeof(tx_wavheader.data_header), tx_wavefile);
            fread(&tx_wavheader.data_bytes, 1, sizeof(tx_wavheader.data_bytes), tx_wavefile);
          }
          if ( (memcmp(tx_wavheader.riff_header, "RIFF", 4U) != 0)
            || (memcmp(tx_wavheader.wave_header, "WAVE", 4U) != 0)
            || (memcmp(tx_wavheader.fmt_header,  "fmt ", 4U) != 0)
            || (memcmp(tx_wavheader.data_header, "data", 4U) != 0) ) {
            fprintf(stderr, "invalid wav file\n");
            fclose(tx_wavefile);
            tx_wavefile = NULL;
            rx_endt = 0; //cancel tx
            continue;
          }
          if ( (tx_wavheader.sample_rate != 8000) || (tx_wavheader.bit_depth != 16) || (tx_wavheader.num_channels != 1) ) {
            fprintf(stderr, "wav file must be 8000Hz 16-bit mono\n");
            fclose(tx_wavefile);
            tx_wavefile = NULL;
            rx_endt = 0; //cancel tx
            continue;
          }
          
          //send header packet
          memset(buf, 0, 56);
          memcpy(buf, header, 4);
          buf[4] = 0x10; //configuration frame
          buf[8] = 0x20; //voice stream
          buf[9] = 0x00;
          buf[10] = 0x01;
          buf[11] = 0x01;
          buf[12] = (tx_streamid >> 8) & 0xFF; //stream id
          buf[13] = tx_streamid & 0xFF; //stream id
          buf[14] = 0x80; //frame counter

          memset(&buf[18], ' ', 8+8+8+8+4);
          memcpy(&buf[18], ref1, (strlen(ref1)<6)?strlen(ref1):6);
          buf[25] = mod1[0];
          memcpy(&buf[26], callsign, 8);
          buf[33] = CLIENT_MOD;
          memcpy(&buf[34], "CQCQCQ", 6);
          memcpy(&buf[42], callsign, 8);
          memcpy(&buf[50], "AI  ", 4);

          addCCITT161(&buf[15], 41);

          for (int i = 0; i < 5; ++i) {
            sendto(udp1, buf, 56, 0, (const struct sockaddr *)&host1, sizeof(host1));
#ifdef DEBUG_SEND
            fprintf(stderr, "SEND %s: ", ref1);
            for(int i = 0; i < 56; ++i)
              fprintf(stderr, "%02x ", buf[i]);
            fprintf(stderr, "\n");
#endif
          }
          
          static const uint8_t ambe_gain[] = {0x61,0x00,0x03,0x00,0x4B,AMBE_ENCODE_GAIN,AMBE_DECODE_GAIN};
          sendto(udp2, ambe_gain, sizeof(ambe_gain), 0, (const struct sockaddr *)&host2, sizeof(host2));

          static const uint8_t ambe_ratep[] = {0x61,0x00,0x0D,0x00,0x0A,0x01,0x30,0x07,0x63,0x40,0x00,0x00,0x00,0x00,0x00,0x00,0x48};
          sendto(udp2, ambe_ratep, sizeof(ambe_ratep), 0, (const struct sockaddr *)&host2, sizeof(host2));
        }
        
        if (tx_wavefile != NULL) {
          //ensure the code below only runs once every 20ms
          struct timespec nanos;
          clock_gettime(CLOCK_MONOTONIC, &nanos);
          int64_t nowus = (int64_t)nanos.tv_sec * 1000000 + nanos.tv_nsec / 1000;
          if (abs(trgus - nowus) > 1000000)
            trgus = nowus;
          if (nowus < trgus)
            continue;
          trgus += 20000;
          //printf("%lld\n", nowus/1000);
          
          uint8_t ambebuf[4+2+320] = {0x61, 0x01, 0x42, 0x02,  0x00, 160};
          if ( fread(&ambebuf[6], 1, 320, tx_wavefile) == 320 ) {
            for (int i=0; i < 160; i++) //swap byte order for all samples, AMBE3000 uses MSB first
              ((unsigned short *)(&ambebuf[6]))[i] = (((unsigned short *)(&ambebuf[6]))[i] >> 8) | (((unsigned short *)(&ambebuf[6]))[i] << 8);
            sendto(udp2, ambebuf, sizeof(ambebuf), 0, (const struct sockaddr *)&host2, sizeof(host2));
          } else {
            fclose(tx_wavefile);
            tx_wavefile = NULL;
            rx_endt = 0;
            printf("*** TX END ***\n");

            //send terminator packet
            memset(buf, 0, 27);
            memcpy(buf, header, 4);
            buf[4] = 0x20; //voice frame
            buf[8] = 0x20; //voice stream
            buf[9] = 0x00;
            buf[10] = 0x01;
            buf[11] = 0x01;
            buf[12] = (tx_streamid >> 8) & 0xFF; //stream id
            buf[13] = tx_streamid & 0xFF; //stream id
            buf[14] = (tx_ambefcnt % 21) | 0x40; //frame counter

            buf[15] = 0x55;
            buf[16] = 0x55;
            buf[17] = 0x55;
            buf[18] = 0x55;
            buf[19] = 0xC8;
            buf[20] = 0x7A;
          
            sendto(udp1, buf, 27, 0, (const struct sockaddr *)&host1, sizeof(host1));
#ifdef DEBUG_SEND
            fprintf(stderr, "SEND %s: ", ref1);
            for(int i = 0; i < 27; ++i)
              fprintf(stderr, "%02x ", buf[i]);
            fprintf(stderr, "\n");
#endif
          }
        }
        
    }
    
    if (time(NULL)-pong_time1 > TIMEOUT) {
      host1_connect = 1;
      fprintf(stderr, "XRF connection timed out, retrying connection...\n");
    }
  }
}
