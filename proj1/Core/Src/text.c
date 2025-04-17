/*
 * text.c
 *
 *  Created on: Mar 9, 2025
 *      Author: citylion
 */

#include "font.h"
#include "display.h"
#include "string.h"

//functions to print characters onto the display one by one from left to right

//origin, the location of the topleft-most pixel where character will start
int originx = 10;
int originy = 10;



//set origin
void origin_set(int x, int y){
	originx=x;
	originy=y;
}

//shifts origin for next character
void origin_next(){
	//originx+=12;
	originx+=14;//leave some space
	if(originx>(255-12)){
		//originy+=14;
		originy+=16;//vertical spacing 2px
		originx=0;
		if(originy>63){
			originy=0;
		}
	}
}

//45 is the first ascii character supported by the font it is character "-"

//write a single ascii character at the current origin values
void write_character(char c){
	if(c<44 || c>96){
		c=96;//whitespace character
	}
		int fontpos = (c-45)*(14); //normalize the ascii value to the font.c array
		int i;
		for( i=0; i<14; i++){
			twelve_write(originx,originy+i,vcr_font[fontpos + i]);
		}
		origin_next();
}

void writestr(char str[]){
	size_t length = strlen(str);
	int i;
	for(int i=0; i<length; i++){
		write_character(str[i]);
	}
}
