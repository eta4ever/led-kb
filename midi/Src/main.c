#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>

#include "usbdrv.h"
#include "oddebug.h"

#include "descriptor.h" // вынес сюда громоздкие константы дескрипторов

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

// test buttons on port C
	PORTC = 0xff;		/* all pull-ups */
	DDRC = 0x00;		/* all pins input */
}


/* Simple monophonic keyboard
   The following function returns a midi note value for the first key pressed.
   Key 0 -> 60 (middle C),
   Key 1 -> 62 (D)
   Key 2 -> 64 (E)
   Key 3 -> 65 (F)
   Key 4 -> 67 (G)
   Key 5 -> 69 (A)
   Key 6 -> 71 (B)
   Key 7 -> 72 (C)
 * returns 0 if no key is pressed.
 */
static uchar keyPressed(void)
{
	uchar i, mask, x;

	x = PINC;
	mask = 1;
	for (i = 0; i <= 13; i += 2) {
		if (6 == i)
			i--;
		if (13 == i)
			i--;
		if ((x & mask) == 0)
			return i + 60;
		mask <<= 1;
	}
	return 0;
}


int main(void)
{
	uchar key, lastKey = 0;
	uchar keyDidChange = 0;
	uchar midiMsg[8];
	uchar iii;

	wdt_enable(WDTO_1S);
	hardwareInit();
	odDebugInit();
	usbInit();

	sendEmptyFrame = 0;

	sei();

	for (;;) {		/* main event loop */
		wdt_reset();
		usbPoll();

		key = keyPressed();
		if (lastKey != key)
			keyDidChange = 1;

		if (usbInterruptIsReady()) {
			if (keyDidChange) {
				/* use last key and not current key status in order to avoid lost
				   changes in key status. */
				// up to two midi events in one midi msg.
				// For description of USB MIDI msg see:
				// http://www.usb.org/developers/devclass_docs/midi10.pdf
				// 4. USB MIDI Event Packets
				iii = 0;
				if (lastKey) {	/* release */
					midiMsg[iii++] = 0x08;
					midiMsg[iii++] = 0x80;
					midiMsg[iii++] = lastKey;
					midiMsg[iii++] = 0x00;
				}
				if (key) {	/* press */
					midiMsg[iii++] = 0x09;
					midiMsg[iii++] = 0x90;
					midiMsg[iii++] = key;
					midiMsg[iii++] = 0x7f;
				}
				if (8 == iii)
					sendEmptyFrame = 1;
				else
					sendEmptyFrame = 0;

				usbSetInterrupt(midiMsg, iii);
				keyDidChange = 0;
				lastKey = key;
			}
		}		// usbInterruptIsReady()
	}
	return 0;
}
