#include <avr/io.h>
#include <avr/pgmspace.h>
/* ������������ ������� ������. ROW+
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

// ������ ���� �������� � ������������ ���

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

// ���������� ��� � 1
void pin_set(int port, char mask){
	port |= mask;
}

// ���������� ��� � 0
void pin_reset(int port, char mask){
	port &= ~mask;
}

// ��������� ���
char pin_read(int port, char mask){
	if (port & mask) return 0;
	return 1;
}

// ������������ ������
char row_scan(uchar row_num){
	pin_set(button_row[row_num].port, button_row[row_num].mask); // ������� ������� �� ���� ������
	char row_byte = 0;

	for (uchar col_num = 0; col_num < COLCOUNT; col_num++){
		if (pin_read(button_row[col_num].port, button_row[col_num].mask)){ // ���� ������� ������� �� ���� �������, �������� ��� � ���� ������
			row_byte &= 1;
		}
		row_byte = row_byte << 1; // �������� ���� ������
	}
	pin_reset(button_row[row_num].port, button_row[row_num].mask); // ������ ������� �� ���� ������
	return row_byte;
}


