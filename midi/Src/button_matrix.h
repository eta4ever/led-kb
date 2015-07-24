#include <avr/io.h>
#include <avr/pgmspace.h>
// Сканирование матрицы кнопок. ROW+

#define ROWCOUNT 2
#define COLCOUNT 2

// сканирование строки
		/* Для сканирования матрицы кнопок. ROW+
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

// разные порты-пины херово объединяются в структуры. Короче, пока ниасилил.

// высокий уровень на пине строки
void row_on(char row_num){
	switch (row_num){
		case 0: PORTB |= (1<<PB4); break;
		case 1: PORTB |= (1<<PB2); break;
		case 2: PORTD |= (1<<PD6); break;
		case 3: PORTB |= (1<<PB0); break;
	}
}

// низкий уровень на пине строки
void row_off(char row_num){
	switch (row_num){
		case 0: PORTB &= ~(1<<PB4); break;
		case 1: PORTB &= ~(1<<PB2); break;
		case 2: PORTD &= ~(1<<PD6); break;
		case 3: PORTB &= ~(1<<PB0); break;
	}
}

char row_scan(char row_num){

	row_on(row_num);

	char row_byte = 0;

	if (PINB & (1<<PB5)) row_byte |= (1<<0);
	if (PINC & (1<<PC2)) row_byte |= (1<<1);
	if (PINC & (1<<PC4)) row_byte |= (1<<2);
	if (PIND & (1<<PD3)) row_byte |= (1<<3);
	if (PINC & (1<<PD1)) row_byte |= (1<<4);

	row_off(row_num);

	return row_byte;
}


