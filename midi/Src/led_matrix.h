#include <avr/io.h>
#include <avr/pgmspace.h>
// Отжигание светодиодами. ROW+
#define ROWCOUNT 4
#define COLCOUNT 5
		/* Для отжигания матрицей светодиодов. ROW+
		 * ROW0 PC0
		 * ROW1 PB3
		 * ROW2 PB1
		 * ROW3 PD7
		 *
		 * COL0 PC1
		 * COL1 PC3
		 * COL2 PC5
		 * COL3 PD5
		 * COL4 PD0
		 */

// разные порты-пины херово объединяются в структуры. Короче, пока ниасилил.

// высокий уровень на пине строки
void led_row_high(char row_num){
	switch (row_num){
		case 0: PORTC |= (1<<PC0); break;
		case 1: PORTB |= (1<<PB3); break;
		case 2: PORTB |= (1<<PB1); break;
		case 3: PORTD |= (1<<PD7); break;
	}
}

// низкий уровень на пине строки
void led_row_low(char row_num){
	switch (row_num){
		case 0: PORTC &= ~(1<<PC0); break;
		case 1: PORTB &= ~(1<<PB3); break;
		case 2: PORTB &= ~(1<<PD1); break;
		case 3: PORTD &= ~(1<<PD7); break;
	}
}

// гашение всех строк
void all_rows_low(void){
	for (char row_num = 0; row_num < ROWCOUNT; row_num++) led_row_low(row_num);
}

void all_rows_high(void){
	for (char row_num = 0; row_num < ROWCOUNT; row_num++) led_row_high(row_num);
}


void led_col_low(char col_num){
	switch (col_num){
		case 0: PORTC &= ~(1<<PC1); break;
		case 1: PORTC &= ~(1<<PC3); break;
		case 2: PORTC &= ~(1<<PC5); break;
		case 3: PORTD &= ~(1<<PD5); break;
		case 4: PORTD &= ~(1<<PD0); break;
	}
}

void led_col_high(char col_num){
	switch (col_num){
		case 0: PORTC |= (1<<PC1); break;
		case 1: PORTC |= (1<<PC3); break;
		case 2: PORTC |= (1<<PC5); break;
		case 3: PORTD |= (1<<PD5); break;
		case 4: PORTD |= (1<<PD0); break;
	}
}

// гашение столбцов
void all_cols_high(void){
	for (char col_num = 0; col_num < COLCOUNT; col_num++) led_col_high(col_num);
}

void all_cols_low(void){
	for (char col_num = 0; col_num < COLCOUNT; col_num++) led_col_low(col_num);
}


void row_flash(char row_num, unsigned char *row, unsigned char tick){

	// По ходу, светодиоды впаял наоборот. Получилось, что
	// на строках активный низкий уровень, на столбцах - высокий

	all_rows_high();
	all_cols_low();

	led_row_low(row_num);

	for (unsigned char led = 0; led < COLCOUNT; led++){

		unsigned char brightness = (*(row+led));
		brightness &= 0b11;

		/* тут такая кустарная ШИМ. tick - счетчик вызова row_flash,
		 * инкрементируется в main. Сбрасывается по достижению 127.
		 * Частота, вроде, приличная получается */

		switch (brightness){

			case 0b11: led_col_high(led); break; //max
			case 0b10: if ( tick < 15 ) led_col_high(led); break; //mid
			case 0b01: if ( tick < 2 ) led_col_high(led); break; //low
		}

		// if (brightness !=0) led_col_high(led);
	}

	// гашение происходит при следующем вызове row_flash

}


