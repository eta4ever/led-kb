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

		// сканирование строк кнопок
		for (uchar row_num = 0; row_num < ROWCOUNT; row_num ++){

			curr_state = row_scan(row_num);

			for (uchar bit=0; bit< COLCOUNT; bit++){ // побитовое сравнение текущего состояния строки с прошлым
				uchar curr_bit = (curr_state >> bit) & 1;
				uchar last_bit = (last_state[row_num] >> bit) & 1;

				if (curr_bit ^ last_bit) continue; // биты равны, переход к следующей итерации

				if (curr_bit) midiMsg[3] = 0x7f; // нажатие: код velocity 0x7f
				else midiMsg[3] = 0x00; // отпускание: код velocity 0x00

				midiMsg[2] = row_num*COLCOUNT + bit; // номер ноты

				sendEmptyFrame = 0; // хз
				if (usbInterruptIsReady()) usbSetInterrupt(midiMsg, 4); // отправка MIDI-сообщения
			}

			last_state[row_num] = curr_state;
		}

	}
	return 0;
}
