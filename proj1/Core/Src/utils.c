/*
 * utils.c
 *
 *  Created on: Feb 28, 2025
 *      Author: citylion
 */
#include "main.h"
#include <stdio.h>
#include "display.h"
#include "text.h"
#include "gps.h"
#include "stm32g4xx_hal.h"
#include "stdlib.h"

/*


#include "display.h"
#include "gps.h"
#include "text.h"
#include "utils.h"

 */

static unsigned int counter = 0;

void maincall(void){

	counter++;
	handleAllNemaMsg();
	//write to the pointer, giving us any message received on uart

	if(counter%10 == 0){
		display_clear();

		if(gps_isJammed){
				origin_set(0,0);
				writestr("GPS JAMMED?");
			}
			else{
				double lon = getLon();
				origin_set(0,14);
				size_t len = snprintf(NULL, 0, "LON: %.3f", lon);
				char *lonStr = malloc(len + 1);
				if (lonStr) {
					snprintf(lonStr, len + 1, "LON: %.3f", lon);
					free(lonStr);
				}
				writestr(lonStr);

				double lat = getLat();
				origin_set(0,28);
				 len = snprintf(NULL, 0, "LAT: %.3f", lat);
				char *latStr = malloc(len + 1);
				if (latStr) {
				    snprintf(latStr, len + 1, "LAT: %.3f", lat);

				    free(latStr);
				}
				writestr(latStr);
			}

		display_update();
	}

}



