/*
 * gps.h
 *
 *  Created on: Apr 11, 2025
 *      Author: citylion
 */

#ifndef INC_GPS_H_
#define INC_GPS_H_


//see gps.c for code details
void initializeGPS(uint8_t start_type);

void startAsyncReceive(_Bool startup);

void transmit(char* c);

void destructiveReadNmeaMsg(char* ptr);

char nmeaChecksum(char* msg);

char parseChecksum(char* msg);

char asciiToLB1(char c);

void requestEasyStatus(void);

void setBaud9600(void);


#endif /* INC_GPS_H_ */
