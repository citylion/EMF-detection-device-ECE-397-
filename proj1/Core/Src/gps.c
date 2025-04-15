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
unsigned char nmeaBuffer[512];//can store several received messages
unsigned char rx_buffer[128];//a single message is ~82

//13 and 10 are end characters

/*
 * start_type value indicates the start-type, with 0 being a cold start
 * which could take 15 minutes for the gps.
 */
void initializeGPS(uint8_t start_type)
{

	HAL_GPIO_WritePin(GPS_RESET_GPIO_Port,GPS_RESET_Pin,0);
	HAL_Delay(100);
	HAL_GPIO_WritePin(GPS_RESET_GPIO_Port,GPS_RESET_Pin,1);

	/*
	 * 2.4.2. PAIR002: PAIR_GNSS_SUBSYS_POWER_ON
	 * Powers on the GNSS system, including DSP, RF, PE and clock.
	 */

	transmit("$PAIR002*38\r\n");

	if(start_type == 0){//cold start

		/*
		 * 2.4.6. PAIR006: PAIR_GNSS_SUBSYS_COLD_START
		 * Performs a cold start, which means that will be erased location information stored in the receiver, including
		   time, position, almanacs and ephemeris data.
		 */
		transmit("$PAIR005*3F\r\n");
	}
	else{//warm start

		/*
		 * Performs a warm start. A warm start means that the GNSS module remembers only rough time, position,
		and almanacs data, and thus needs to download an ephemeris before it can get a valid position.
		 */

		transmit("$PAIR006*3C\r\n");
	}

}



void startAsyncReceive(_Bool startup){
	//first, put the prior message in the main buffer
	unsigned int buffSize = sizeof(nmeaBuffer);//size of the main buffer,

	if(!startup){

		for(int i=0; i<128; i++){
				if(writePos >= buffSize){
					writePos=0;
				}
				char c = rx_buffer[i];
				nmeaBuffer[writePos] = c;
				writePos++;

				if(c == '\n'){
					break;
				}
		}
	}

	HAL_UARTEx_ReceiveToIdle_IT(&huart1, rx_buffer, sizeof(rx_buffer));
}

void transmit(char* c)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)c, strlen(c), HAL_MAX_DELAY);
}


//gets the first nmea message found in the main large buffer "buffer"
//then overwrites that message so it is not read again by the next
//call to this function
void destructiveReadNmeaMsg(char* ptr){
	unsigned int pos = 0;

	_Bool start = 0;

	unsigned int rwPos1 = 0;
	unsigned int rwPos2 = 0;
	int buffSize = sizeof(nmeaBuffer);//size of the main buffer,
	//we are going to iterate over the entire buffer looking for a message:
	for(int i=0; i<2*buffSize;i++){//the 2*size is intentional, since a message may start at the end of the buffer and "wrap" over, something more like 1.2x is probably more realistic
		if(i>buffSize && !start){
			break;//if we are wrapped back around and still didnt find a character, then there is no message.
		}
		char c = nmeaBuffer[i%buffSize];
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
			nmeaBuffer[i] = 255;
			i++;
			if(i > rwPos2){
				break;
			}
			if(i>buffSize){
				i=0;
			}
		}
	}
	ptr[pos] = '\0';//null terminator

}

void setBaud9600(void){
	transmit("$PAIR864,0,0,9600*13\r\n");
}


/*
 * Certain messages require certain actions, others do not
 * For example, we want to save gps data if the message type is a gps message
 * we also want to save jamming status of the gps chip.
 */
void handleNemaMsg(char* ptr){
	//unfinished
	short chk1 = nmeaChecksum(ptr);

}

void handleAllNemaMsg(){
	//unfinished
}

/*
 * 2.4.47. PAIR491: PAIR_EASY_GET_STATUS
	Queries the status of the EASY function.

	Type:
	Get

	Synopsis:
	$PAIR491*<Checksum><CR><LF>
 */

void requestEasyStatus(void){

	transmit("$PAIR491*36\r\n");

}

void requestDebugLog(void){

	transmit("$PAIR086,1*29\r\n");

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
			msb = asciiToLB1(c);
		}
		else if(found == 1 && msb != 127){
			lsb = asciiToLB1(c);
		}
		else if(found == 1){
			break;
		}
		i++;
	}


	combined = (msb<<4) | lsb;
	return combined;

}

char asciiToLB1(char c){
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














