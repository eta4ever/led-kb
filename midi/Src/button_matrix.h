#include <avr/io.h>
#include <avr/pgmspace.h>
/* Сканирование матрицы кнопок. ROW+
 * ROW0 PB4
 * ROW1 PB2
 * ROW2 PD6
 * ROW3 PB0
 *
 * COL0 PB5
 * COL1 PC2
 * COL2 PC4
 * COL3 PD3
 * COL4 PD1
 */

#define ROWCOUNT 2
#define COLCOUNT 2

// сканирование строки
char row_scan(char row_num){

	switch (row_num){
		case 0: PORTB |= 0x00010000; break;
		case 1: PORTB |= 0x00000100; break;
	}

	char row_byte = 0;

	if (PINB & 0b00100000) row_byte |= 0b01;
	if (PINC & 0b00000100) row_byte |= 0b10;

//	switch (row_num){
//		case 0: PORTB &= 0x11101111; break;
//		case 1: PORTB &= 0x11111011; break;
//	}

	return row_byte;
}


