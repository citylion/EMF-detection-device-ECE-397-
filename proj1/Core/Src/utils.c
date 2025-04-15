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

/*


#include "display.h"
#include "gps.h"
#include "text.h"
#include "utils.h"

 */

static unsigned int counter = 0;

void maincall(void){

	counter++;

	char msg[82];
	destructiveReadNmeaMsg(msg);//give the function the pointer to msg, it will
	//write to the pointer, giving us any message received on uart
	if(msg[0] == '$'){

	  origin_set(0, 0);
	  for(int i=0; i<82; i++){
		  char c = msg[i];
		  if(c==10 || c==13 || c == '\0'){
			  break;
		  }
		  write_character(c);
	  }
	}
	if(counter%50 == 0){
		display_update();
		requestEasyStatus();
	}



}



