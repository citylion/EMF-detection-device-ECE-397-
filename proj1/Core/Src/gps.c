/*
 * gps.c
 *
 *  Created on: Apr 11, 2025
 *      Author: citylion
 */

#include "main.h"
#include "stdio.h"
#include "string.h"
#include "gps.h"

extern UART_HandleTypeDef huart1;


unsigned int writePos = 0;
unsigned char nmea_buffer[1024];//can store several received messages
unsigned char rx_buffer[1024];//a single message is ~82
double lat = -1;
double lon = -1;
_Bool gpsJammed = 0;

//13 and 10 are end characters

/*
 * start_type value indicates the start-type, with 0 being a cold start
 * which could take 15 minutes for the gps.
 */
void initializeGPS(uint8_t start_type)
{

	nmea_buffer[1023] = '\0';
	rx_buffer[1023] = '\0';

	HAL_GPIO_WritePin(GPS_RESET_GPIO_Port,GPS_RESET_Pin,0);
	HAL_Delay(100);
	HAL_GPIO_WritePin(GPS_RESET_GPIO_Port,GPS_RESET_Pin,1);



}



void startAsyncReceive(_Bool startup){


	if(!startup){

		//first, put the prior message in the main buffer
		unsigned int buffSize = sizeof(nmea_buffer);//size of the main buffer,
		unsigned int rxBuffSize = sizeof(rx_buffer);
		while(strchr(rx_buffer,'$') != NULL){
			char* ptr = strchr(rx_buffer,'$');
			int i=0;
			while(1){
				if(ptr[i] == '\0'){
					break; break;
				}
				else{
					if(writePos >= buffSize-1){
						writePos=0;
					}
					nmea_buffer[writePos] = ptr[i];
					writePos++;
				}
				if(ptr[i] == '\n'){
						break;
				}
				i++;
			}
		}
	}

	HAL_UARTEx_ReceiveToIdle_DMA(&huart1, rx_buffer, sizeof(rx_buffer));
}

void transmit(char* c)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)c, strlen(c), HAL_MAX_DELAY);
}


//gets the first nmea message found in the main large buffer "buffer"
//then overwrites that message so it is not read again by the next
//call to this function
_Bool destructiveReadNmeaMsg(char* ptr){
	unsigned int pos = 0;

	_Bool start = 0;

	unsigned int rwPos1 = 0;
	unsigned int rwPos2 = 0;
	int buffSize = sizeof(nmea_buffer);//size of the main buffer,
	//we are going to iterate over the entire buffer looking for a message:
	for(int i=0; i<2*buffSize-1;i++){//the 2*size is intentional, since a message may start at the end of the buffer and "wrap" over, something more like 1.2x is probably more realistic
		if(i>buffSize && !start){
			break;//if we are wrapped back around and still didnt find a character, then there is no message.
		}
		char c = nmea_buffer[i%buffSize];
		if(c == '$'){
			rwPos1=i%buffSize;
			start=1;
		}
		if(start==1){
			ptr[pos] = c;pos++;
		}
		if(start==1 && c == 10){
			rwPos2=i%buffSize;
			break;
		}
	}
	if(start){
		//first overwrite the read msg
		int i=rwPos1;
		while(1){
			nmea_buffer[i] = 255;
			i++;
			if(i > rwPos2){
				break;
			}
			if(i>=buffSize){
				i=0;
			}
		}
		ptr[pos] = '\0';//null terminator
	}
	return start;
}


/*
 * Certain messages require certain actions, others do not
 * For example, we want to save gps data if the message type is a gps message
 * we also want to save jamming status of the gps chip.
 */
void handleNmeaMsg(char* msg){
	//unfinished
	char last = 1;
	char* ptr = strstr(msg,"$GNRMC");
	if(ptr != NULL){
		//pos=20th char is start of lat
		//pos=32th char is direction N/S character
		//pos=34th char is start of lon
		//pos=47 is the direction W/E character

		int origin = 20;
		if(ptr[origin] == ','){
			gpsJammed = 1; return;
		}
		else{
			gpsJammed = 0;
		}


		int sublat=10*asciiToHex(ptr[origin]) + 1*asciiToHex(ptr[origin+1]);         // V intentional skip +4
		float latminutes = ( 10*asciiToHex(ptr[origin+2]) + 1*asciiToHex(ptr[origin+3]) + 0.1*asciiToHex(ptr[origin+5]) + 0.01*asciiToHex(ptr[origin+6]) + 0.001*asciiToHex(ptr[origin+7])) / 60;
		lat = sublat + latminutes;

		origin=32;
		if(ptr[origin] == 'S'){ lat = lat*-1;}


		origin = 34;
		if(ptr[origin] == ','){
			gpsJammed = 1; return;
		}
		else{
			gpsJammed = 0;
		}

		int sublon=10*asciiToHex(ptr[origin]) + 1*asciiToHex(ptr[origin+1]);         // V intentional skip +4
		float lonminutes = ( 10*asciiToHex(ptr[origin+2]) + 1*asciiToHex(ptr[origin+3]) + 0.1*asciiToHex(ptr[origin+5]) + 0.01*asciiToHex(ptr[origin+6]) + 0.001*asciiToHex(ptr[origin+7])) / 60;
		lon = sublon + lonminutes;

		origin = 47;
		if(ptr[origin] == 'W'){ lon = lon*-1; }
	}

}

void handleAllNemaMsg(){
	while(strchr(nmea_buffer,'$') != NULL){
		char msg[82];

		_Bool success = destructiveReadNmeaMsg(msg);
		if(success){
			handleNmeaMsg(msg);
		}
	}
}

//given any nema message, parses out the address and value
//segments, and calculates the checksum
//and returns nema checksum (1 byte)
char nmeaChecksum(char* msg){

	char chk = 0;

	int i=0;
	if(msg[0] == '$'){//skip if so, as this character is excluded
		i++;
	}
	size_t stop = strlen(msg)+1;
	while(i<stop){
		if(msg[i] == '*'){
			break;
		}
		else if(msg[i] == '\0'){
			break;
		}
		else{
			chk ^= (char)msg[i];
		}
		i++;
	}

	return chk;

}

//given only a full NEMA message, parses out the checksum value.
//Returns 1 byte (checksum from ascii calculation)
char parseChecksum(char* msg){

	size_t stop = strlen(msg)+1;
	int i=0;
	_Bool found = 0;
	char msb = 127;//only using lower 4 bits
	char lsb = 127;//only using lower 4 bits
	char combined = 0;// 8 bits = msb (shifted up 4) + lsb
	while(i<stop){
		char c = msg[i];
		if(c == '*'){
			found=1;
		}
		else if(found == 1 && msb == 127){
			msb = asciiToHex(c);
		}
		else if(found == 1 && msb != 127){
			lsb = asciiToHex(c);
		}
		else if(found == 1){
			break;
		}
		i++;
	}


	combined = (msb<<4) | lsb;
	return combined;

}

uint8_t asciiToHex(char c){
	switch(c){
		case '0': return 0; break;
		case '1': return 1; break;
		case '2': return 2; break;
		case '3': return 3; break;
		case '4': return 4; break;
		case '5': return 5; break;
		case '6': return 6; break;
		case '7': return 7; break;
		case '8': return 8; break;
		case '9': return 9; break;
		case 'A': return 10; break;
		case 'B': return 11; break;
		case 'C': return 12; break;
		case 'D': return 13; break;
		case 'E': return 14; break;
		case 'F': return 15; break;
		default: return 16; break;
	}

}

_Bool gps_isJammed(void){
	return gpsJammed;
}

double getLon(void){
	return lon;
}

double getLat(void){
	return lat;
}

















