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

	uint8_t BUT_current = PIND & (1 << PD1);; // текущее значение кнопки
	uint8_t BUT_previous = BUT_current; // предыдущее значение кнопки
	//uint8_t BUT_message=0; // 0 - ничего не отправлять, 1 - отправить vel 0, 2 - отправить vel 127
	//uint8_t LED_current = 0; // текущее значение светодиода

	uint16_t ADC_current = raw_ADC(); // текущее значение АЦП
	uint16_t ADC_previous = ADC_current; // предыдущее значение АЦП
	uint8_t ADC_deviation = 7; // порог фиксации изменения АЦП, давить шум

	for (;;)
	{

		MIDI_EventPacket_t ReceivedMIDIEvent; // прием
		
		// обработка входящих сообщений
		while (MIDI_Device_ReceiveEventPacket(&Keyboard_MIDI_Interface, &ReceivedMIDIEvent)) 
		{
			// if ((ReceivedMIDIEvent.Event == MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON)) && (ReceivedMIDIEvent.Data3 > 0))
			//   LEDs_SetAllLEDs(ReceivedMIDIEvent.Data2 > 64 ? LEDS_LED1 : LEDS_LED2);
			// else
			//   LEDs_SetAllLEDs(LEDS_NO_LEDS);

			// if ((ReceivedMIDIEvent.Event == MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON)) && (ReceivedMIDIEvent.Data2 = 0b00000001))
			// {
			// 	if (ReceivedMIDIEvent.Data3 == 0) led_off(); else if (ReceivedMIDIEvent.Data3 == 127) led_on();
			// }

			if ((ReceivedMIDIEvent.Event == MIDI_EVENT(0, MIDI_COMMAND_CONTROL_CHANGE)) && (ReceivedMIDIEvent.Data2 == 0b00000001))
			{
				if (ReceivedMIDIEvent.Data3 == 0) led_off(); else if (ReceivedMIDIEvent.Data3 == 127) led_on();
			}

		}

		// обработка потенциометра
		ADC_current = raw_ADC();
		if ( abs(ADC_current - ADC_previous) >= ADC_deviation ) // если изменения больше порога
		{
			// MIDI_EventPacket_t MIDIEvent = (MIDI_EventPacket_t) // сформировать пакет
			// {
			// 	.Event       = MIDI_EVENT(0, MIDI_COMMAND_CONTROL_CHANGE), // VirtualCable 0
			// 	.Data1       = MIDI_COMMAND_CONTROL_CHANGE | MIDI_CHANNEL(1),
			// 	.Data2       = 0b00000001, // контроллер 1
			// 	.Data3       = ADC_current / 8, // с АЦП приходит 0-1023, а надо выдать 0-127
			// };

			// MIDI_Device_SendEventPacket(&Keyboard_MIDI_Interface, &MIDIEvent); // отправить пакет
			// MIDI_Device_Flush(&Keyboard_MIDI_Interface);

			cc_send(2, ADC_current / 8);

			ADC_previous = ADC_current;
		}

		// обработка кнопки
		else 
		{

			BUT_current = PIND & (1 << PD1);
			if (BUT_current != BUT_previous) { // по первому нажатию CC#1 127, по второму 0 
				// noteon_send(127);
				// noteon_send(0);
				if (BUT_current) { cc_send(1,127); } else { cc_send(1,0); }
				BUT_previous = BUT_current;
			} 
		}

		MIDI_Device_USBTask(&Keyboard_MIDI_Interface);
		USB_USBTask();
	}
}

// отправить в кабель 0 канал 1 NOTE_ON с заданным vel
void noteon_send(vel)
{
	MIDI_EventPacket_t MIDIEvent = (MIDI_EventPacket_t) // сформировать пакет
	{
		.Event       = MIDI_EVENT(0, MIDI_COMMAND_NOTE_ON), // VirtualCable 0
		.Data1       = MIDI_COMMAND_NOTE_ON | MIDI_CHANNEL(1),
		.Data2       = 0b00000001,
		.Data3       = vel, 
	};

	MIDI_Device_SendEventPacket(&Keyboard_MIDI_Interface, &MIDIEvent); // отправить пакет
	MIDI_Device_Flush(&Keyboard_MIDI_Interface);
}

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

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	LEDs_Init();
	USB_Init();

	// ----------------D2 (PD1) вход кнопки, D3 (PD0) выход на светодиод
	
	DDRD &= ~(1 << PD1);
	DDRD |= (1 << PD0);

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

// зажечь светодиод
void led_on(void)
{
	PORTD |= (1 << PD0);
}

// погасить светодиод
void led_off(void)
{
	PORTD &= ~(1 << PD0);
}


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

