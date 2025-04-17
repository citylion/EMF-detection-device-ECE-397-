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

char nmeaChecksum(char* msg);

char parseChecksum(char* msg);

uint8_t asciiToHex(char c);

_Bool gps_isJammed(void);

double getLon(void);

double getLat(void);

void handlePositionMsg(char* msg);


#endif /* INC_GPS_H_ */
