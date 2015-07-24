#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <avr/delay.h>

#include "usbdrv.h"
#include "oddebug.h"

#include "descriptor.h" // вынес сюда громоздкие константы дескрипторов

#include "button_matrix.h" // обработка матрицы кнопок
#include "led_matrix.h" // обработка матрицы светодиодов

uchar last_state[ROWCOUNT] = { 0 }; // массив предыдущего состояния матрицы кнопок, строка - байт
uchar curr_state = 0; // текущее состояние строки матрицы кнопок

uchar midiMsg[4] = { 0x09, 0x90, 0x00, 0x00 }; // MIDI-сообщение. Только Note On. Изменяются 3-4 байты (key и velocity)

uchar leds[ROWCOUNT] = { 0 }; // массив состояния светодиодов
//uchar lastkey = 0;
//uchar currkey = 0;

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */
uchar usbFunctionDescriptor(usbRequest_t * rq) {
  if (rq->wValue.bytes[1] == USBDESCR_DEVICE) {
    usbMsgPtr = (uchar *) deviceDescrMIDI;
    return sizeof(deviceDescrMIDI);
  } else {		/* must be config descriptor */
    usbMsgPtr = (uchar *) configDescrMIDI;
    return sizeof(configDescrMIDI);
  }
}

static uchar sendEmptyFrame;

uchar usbFunctionSetup(uchar data[8]) {
  usbRequest_t    *rq = (void *)data;

  if((rq->bmRequestType & USBRQ_TYPE_MASK) == USBRQ_TYPE_CLASS){    /* class request type */

    /*  Prepare bulk-in endpoint to respond to early termination   */
    if ((rq->bmRequestType & USBRQ_DIR_MASK) ==
	USBRQ_DIR_HOST_TO_DEVICE)
      sendEmptyFrame = 1;
  }
  return 0xff;
}

/*---------------------------------------------------------------------------*/
/* usbFunctionRead                                                           */
/*---------------------------------------------------------------------------*/

uchar usbFunctionRead(uchar * data, uchar len)
{
	data[0] = 0;
	data[1] = 0;
	data[2] = 0;
	data[3] = 0;
	data[4] = 0;
	data[5] = 0;
	data[6] = 0;

	return 7;
}


/*---------------------------------------------------------------------------*/
/* usbFunctionWrite                                                          */
/*---------------------------------------------------------------------------*/

uchar usbFunctionWrite(uchar * data, uchar len)
{
	return 1;
}

/*---------------------------------------------------------------------------*/
/* usbFunctionWriteOut                                                       */
/*                                                                           */
/* this Function is called if a MIDI Out message (from PC) arrives.          */
/*                                                                           */
/*---------------------------------------------------------------------------*/

uchar usbFunctionWriteOut(uchar * data, uchar len)
{
/*
 * USB-MIDI пакет нам нужен только четырехбайтовый.
 * Первый байт - старший полубайт Cable Number, игнорируем
 * Первый байт - младший полубайт Code Index Number (cin). Интересны 0x9 (Note On) и 0x8 (Note Off).
 * MIDI-сообщения вида 9n kk vv оборачивается в USB-MIDI Event вида 19 9n kk vv (для Cable Number 1 CIN 9)
 *
 */
	uchar cin = (*data) & 0x0f; // игнор старшего полубайта, нужен только младший
	if (( cin != 0x9 ) && ( cin != 0x8 )) return 0; // возврат 0 если "ненужный" пакет

	uchar note = (*data + 2);
//	uchar vel = (*data + 3); пока не используется
	uchar row_num = note / COLCOUNT;
	uchar col_num = note % COLCOUNT;

	if (cin == 0x9) leds[row_num] |= (1<<col_num); // включить бит в матрице
	if (cin == 0x8) leds[row_num] &= ~(1<<col_num); // выключить бит в матрице

//	if (len > 4) parseUSBMidiMessage(data+4, len-4); предусмотреть потом обработку нескольких миди-сообщений в одном USB,

	return 1;
}

/*---------------------------------------------------------------------------*/
/* hardwareInit                                                              */
/*---------------------------------------------------------------------------*/

static void hardwareInit(void)
{
	uchar i, j;

	/* activate pull-ups except on USB lines */
	USB_CFG_IOPORT =
	    (uchar) ~ ((1 << USB_CFG_DMINUS_BIT) |
		       (1 << USB_CFG_DPLUS_BIT));
	/* all pins input except USB (-> USB reset) */
#ifdef USB_CFG_PULLUP_IOPORT	/* use usbDeviceConnect()/usbDeviceDisconnect() if available */
	USBDDR = 0;		/* we do RESET by deactivating pullup */
	usbDeviceDisconnect();
#else
	USBDDR = (1 << USB_CFG_DMINUS_BIT) | (1 << USB_CFG_DPLUS_BIT);
#endif

	j = 0;
	while (--j) {		/* USB Reset by device only required on Watchdog Reset */
		i = 0;
		while (--i);	/* delay >10ms for USB reset */
	}
#ifdef USB_CFG_PULLUP_IOPORT
	usbDeviceConnect();
#else
	USBDDR = 0;		/*  remove USB reset condition */
#endif


DDRB |= 0b00010101; // PB4,2,0 - выходы
DDRD |= 0b01000000; // PD6 выход
DDRB &= 0b11011111; // PB5 вход
DDRC &= 0b11101011; // PC4,2 - входы
DDRD &= 0b11110101; // PD3,1 - входы

}

int main(void)
{
	wdt_enable(WDTO_1S);
	hardwareInit();
	odDebugInit();
	usbInit();

	sendEmptyFrame = 0;

	sei();

	for (;;) {		/* main event loop */
		wdt_reset();
		usbPoll();

		for (uchar row_num = 0; row_num < ROWCOUNT; row_num++){

			// сканирование матрицы кнопок
			curr_state = row_scan(row_num);
			if (curr_state == last_state[row_num]) continue;

			for (uchar bit = 0; bit < COLCOUNT; bit++){

				uchar curr_bit = (curr_state >> bit) & 1;
				uchar last_bit = (last_state[row_num] >> bit) & 1;

				if ( curr_bit == last_bit ) continue;

				if ( curr_bit ) midiMsg[3] = 0x7f;
				else midiMsg[3] = 0x00;

				midiMsg[2] = bit + row_num * COLCOUNT;

				if (usbInterruptIsReady()) usbSetInterrupt(midiMsg, 4);

				_delay_ms(10);
			}

			last_state[row_num] = curr_state;

			// матрица светодиодов
			row_flash(row_num, leds[row_num]);

		}

	}
	return 0;
}
