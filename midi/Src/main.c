#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "usbdrv.h"
#include "oddebug.h"

#include "descriptor.h" // вынес сюда громоздкие константы дескрипторов

#include "button_matrix.h" // обработка матрицы кнопок

uchar last_state[ROWCOUNT] = { 0 }; // массив предыдущего состояния матрицы кнопок, строка - байт
uchar curr_state = 0; // текущее состояние строки матрицы кнопок

uchar midiMsg[4] = { 0x09, 0x90, 0x00, 0x00 }; // MIDI-сообщение. Только Note On. Изменяются 3-4 байты (key и velocity)

uchar lastkey = 0;
uchar currkey = 0;

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

void usbFunctionWriteOut(uchar * data, uchar len)
{
//
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

		PORTB |= 0b00010000;
		curr_state = 0;

		if (PINB & 0b00100000) curr_state |= 0b01;
		if (PINC & 0b00000100) curr_state |= 0b10;

		if ( (curr_state & 0b01) != (last_state[0] & 0b01) ){

			midiMsg[2] = 0;
			if (curr_state & 0b01) midiMsg[3] = 0x7f;
			else midiMsg[3] = 0;

			if (usbInterruptIsReady()) usbSetInterrupt(midiMsg, 4);
		}

		if ( (curr_state & 0b10) != (last_state[0] & 0b10) ){

			midiMsg[2] = 1;
			if (curr_state & 0b10) midiMsg[3] =  0x7f;
			else midiMsg[3] = 0;

			if (usbInterruptIsReady()) usbSetInterrupt(midiMsg, 4);
		}

		last_state[0] = curr_state;

		PORTB &= 0b11101111;
		PORTB |= 0b00000100;
		curr_state = 0;

		if (PINB & 0b00100000) curr_state |= 0b01;
		if (PINC & 0b00000100) curr_state |= 0b10;

		if ( (curr_state & 0b01) != (last_state[1] & 0b01) ){

			midiMsg[2] = 2;
			if (curr_state & 0b01) midiMsg[3] = 0x7f;
			else midiMsg[3] = 0;

			if (usbInterruptIsReady()) usbSetInterrupt(midiMsg, 4);
		}

		if ( (curr_state & 0b10) != (last_state[1] & 0b10) ){

			midiMsg[2] = 3;
			if (curr_state & 0b10) midiMsg[3] =  0x7f;
			else midiMsg[3] = 0x0;

			if (usbInterruptIsReady()) usbSetInterrupt(midiMsg, 4);
		}

		last_state[1] = curr_state;
		PORTB &= 0b11111011;

	}
	return 0;
}
