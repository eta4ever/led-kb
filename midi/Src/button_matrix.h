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

//const char ROWCOUNT = 4;
//const char COLCOUNT = 5;
#define ROWCOUNT 4
#define COLCOUNT 5

// свести этот разнобой в удобоваримый вид

struct port_pin { int port; char mask; };

PROGMEM static const struct port_pin button_row[] = {

		{0x38, 4}, //PB4
		{0x38, 2}, //PB2
		{0x32, 6}, //PD6
		{0x38, 0}, //PB0
};

PROGMEM static const struct port_pin button_col[] = {

		{0x36, 5},
		{0x33, 2},
		{0x33, 4},
		{0x30, 3},
		{0x30, 1},
};

// установить бит в 1
void pin_set(int port, char mask){
	port |= mask;
}

// установить бит в 0
void pin_reset(int port, char mask){
	port &= ~mask;
}

// прочитать бит
char pin_read(int port, char mask){
	if (port & mask) return 0;
	return 1;
}

// сканирование строки
char row_scan(uchar row_num){
	pin_set(button_row[row_num].port, button_row[row_num].mask); // высокий уровень на пине строки
	char row_byte = 0;

	for (uchar col_num = 0; col_num < COLCOUNT; col_num++){
		if (pin_read(button_row[col_num].port, button_row[col_num].mask)){ // если высокий уровень на пине столбца, добавить бит в байт строки
			row_byte &= 1;
		}
		row_byte = row_byte << 1; // сдвинуть байт строки
	}
	pin_reset(button_row[row_num].port, button_row[row_num].mask); // низкий уровень на пине строки
	return row_byte;
}


