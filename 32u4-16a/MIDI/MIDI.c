/*
             LUFA Library
     Copyright (C) Dean Camera, 2015.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2015  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include "MIDI.h"
#include <util/twi.h>

#define F_CPU 16000000UL

/** LUFA MIDI Class driver interface configuration and state information. This structure is
 *  passed to all MIDI Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_MIDI_Device_t Keyboard_MIDI_Interface =
	{
		.Config =
			{
				.StreamingInterfaceNumber = INTERFACE_ID_AudioStream,
				.DataINEndpoint           =
					{
						.Address          = MIDI_STREAM_IN_EPADDR,
						.Size             = MIDI_STREAM_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint          =
					{
						.Address          = MIDI_STREAM_OUT_EPADDR,
						.Size             = MIDI_STREAM_EPSIZE,
						.Banks            = 1,
					},
			},
	};


/*	MIDI

	Event = MIDI_EVENT(0, MIDI_COMMAND_CONTROL_CHANGE)
	Собственно, испольуем только MIDI_COMMAND_CONTROL_CHANGE и MIDI_COMMAND_NOTE_ON
	0 - это Virtual Cable. Это не трогаем. 

	Data1 - байт статуса. xxxxyyyy, где xxxx - команда, yyyy - канал. Пример .Data1 = MIDI_COMMAND_CONTROL_CHANGE | MIDI_CHANNEL(1)
	Data2-3 - данные. Для NOTE_ON Data2 - номер ноты, Data3 - Velocity. Старший бит всегда 0, поэтому 0-127.
	Для CC Data2 - номер контроллера (120-127 зарезервированы!), Data3 - значение.

	http://www.midi.org/techspecs/midimessages.php

*/


int main(void)
{
	SetupHardware();

	// LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
	GlobalInterruptEnable();

	// uint8_t BUT_current = PIND & (1 << PD1);; // текущее значение кнопки
	// uint8_t BUT_previous = BUT_current; // предыдущее значение кнопки
	//uint8_t BUT_message=0; // 0 - ничего не отправлять, 1 - отправить vel 0, 2 - отправить vel 127
	//uint8_t LED_current = 0; // текущее значение светодиода

	uint16_t ADC_current[16]; // текущие значения АЦП
	uint16_t ADC_previous[16]; // предыдущие значения АЦП
	uint8_t ADC_deviation = 7; // порог фиксации изменения АЦП, давить шум

	for (uint8_t i=0; i<16, i++) { ADC_current[i] = 0; ADC_previous[i] = 0;} // обнуление

	for (;;)
	{
		// опрос 16 входов мультиплексора
		for (uint8_t MUX_pos = 0; MUX_pos < 16; MUX_pos++ ){

				MUX_address(MUX_pos); // установить адрес
				
				ADC_current[MUX_pos] = raw_ADC(); // АЦП
				if ( abs(ADC_current[MUX_pos] - ADC_previous[MUX_pos]) >= ADC_deviation ) // если изменения больше порога, отправить сообщение CC
				{
					cc_send(2, ADC_current[MUX_pos] / 8);
					ADC_previous[MUX_pos] = ADC_current[MUX_pos];
				}

				MIDI_Device_USBTask(&Keyboard_MIDI_Interface);
				USB_USBTask();
		}
	}
}

// // отправить в кабель 0 канал 1 NOTE_ON с заданным vel
// void noteon_send(vel)
// {
// 	MIDI_EventPacket_t MIDIEvent = (MIDI_EventPacket_t) // сформировать пакет
// 	{
// 		.Event       = MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON), // VirtualCable 0
// 		.Data1       = MIDI_COMMAND_NOTE_ON | MIDI_CHANNEL(1),
// 		.Data2       = 0b00000001,
// 		.Data3       = vel, 
// 	};

// 	MIDI_Device_SendEventPacket(&Keyboard_MIDI_Interface, &MIDIEvent); // отправить пакет
// 	MIDI_Device_Flush(&Keyboard_MIDI_Interface);
// }

// отправить в кабель 0 инструмент instr CONTROL CHANGE с заданным vel
void cc_send(instr, vel)
{
	MIDI_EventPacket_t MIDIEvent = (MIDI_EventPacket_t) // сформировать пакет
			{
				.Event       = MIDI_EVENT(0, MIDI_COMMAND_CONTROL_CHANGE), // VirtualCable 0
				.Data1       = MIDI_COMMAND_CONTROL_CHANGE | MIDI_CHANNEL(1), // канал 1
				.Data2       = instr, 
				.Data3       = vel, 
			};

			MIDI_Device_SendEventPacket(&Keyboard_MIDI_Interface, &MIDIEvent); // отправить пакет
			MIDI_Device_Flush(&Keyboard_MIDI_Interface);
}

/** установить адрес входа мультиплексора */
void MUX_address(uint8_t address)
{
	if (address & 0b0001) PORTF |= (1<<PF4) else PORTF &= ~(1<<PF4);
	if (address & 0b0010) PORTF |= (1<<PF5) else PORTF &= ~(1<<PF5);
	if (address & 0b0100) PORTF |= (1<<PF6) else PORTF &= ~(1<<PF6);
	if (address & 0b1000) PORTF |= (1<<PB6) else PORTF &= ~(1<<PB6);
}


/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	//LEDs_Init();
	USB_Init();

	// ----------------D2 (PD1) вход кнопки, D3 (PD0) выход на светодиод
	// PB6, PF6, PF5, PF4 - выходы адреса мультиплексора, PF4-S0 ... PB6-S3
	// PB5 - выход разрешения мультиплексора (активный низкий)
	DDRB |= (1<<PB5) | (1<<PB6);
	DDRF |= (1<<PF6) | (1<<PF5) | (1<<PF4);

	// DDRD &= ~(1 << PD1);

	// разрешить работу мультиплексора
	PORTB &= ~(1<<PB5);
	
	// ---------------- настройка АЦП------------------------------------------

	ADMUX &= ~((1 << REFS1)); // REFS0 = 1, REFS1 = 0 - используется AVCC, оно VCC (на Pro Micro)
	ADMUX |= (1 << REFS0);

	ADMUX &= ~((1 << MUX5) | (1 << MUX4) | (1 << MUX3)); // 000111 - выбор ADC7 без извращений (A0 на Pro Micro)
	ADMUX |= ((1<< MUX2) | (1<< MUX1) | (1<< MUX0));

	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0); // 111 делитель 128, частота АЦП 16000/128 = 125 КГц
	ADCSRA |= (1 << ADEN); // включение АЦП
}

// считать АЦП
int raw_ADC(void)
{
	ADCSRA |= (1 << ADSC); // начать преобразование
	while (ADCSRA & (1 << ADSC)); // ждать сброса бита - окончания преобразования
	return ADC;
}

// // зажечь светодиод
// void led_on(void)
// {
// 	PORTD |= (1 << PD0);
// }

// // погасить светодиод
// void led_off(void)
// {
// 	PORTD &= ~(1 << PD0);
// }


/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	// LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	// LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= MIDI_Device_ConfigureEndpoints(&Keyboard_MIDI_Interface);

	// LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	MIDI_Device_ProcessControlRequest(&Keyboard_MIDI_Interface);
}

