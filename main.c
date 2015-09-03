//JUST FINISHED UP TO GETTING IN IDLE STATE (UNTESTED)
//ADDED: Function "sendFrame()" and "sendByte()" which should be done with operation (IFGs checked) by the time they are done
//NEED TO: first, check that we actually get to idle state.

#include <msp430g2553.h>


#define SDCS 0x08
#define SIMO 0x04
#define SOMI 0x02
#define CLK 0x10
#define CRC 0x95
#define GOOD 0x40
#define BAD 0x01
#define PWR 0x20


int errorCode=0;

//__interrupt void USCIAB0RX_ISR (void);

void sendByte(int txByte){
	UCA0TXBUF=txByte;
	while(!(IFG2 & UCA0TXIFG)){}
	while(IFG2 & UCA0RXIFG){}
}

void sendFrame(int cmd, long cmdArg){
	sendByte(1<<6+cmd);		//Frame up the command
	sendByte(cmdArg>>24);	//Send the four byte argument, MSB fist
	sendByte(cmdArg>>16);
	sendByte(cmdArg>>8);
	sendByte(cmdArg);
	sendByte(CRC);			//Send the CRC command (only important for SDC SW Reset)
	//Send 0xff on the TX for NCR frames/until we get a response from the SDC!
	int timeout=0;
	while(errorCode & 0x80){
		sendByte(0xFF);
		timeout++;
		if(timeout==15){
			P1OUT |= BAD;
			break;
		}
	}
//	P1OUT |= GOOD;
}

int main(void) {
	WDTCTL = WDTPW | WDTHOLD;		// Stop watchdog timer

	//Initialize SD Card
	//Power ON Procedure:
	//(Configure pins for initialization)
	P1DIR |= (GOOD + BAD + SDCS + SIMO + SOMI + CLK + PWR);
	P1OUT &= ~(SOMI + CLK + SIMO + GOOD + BAD + PWR);
	P1OUT |= SDCS;
	//(To have an accurate delay/clock time, set clock to ~1.6MHz)
	DCOCTL |= DCO1 + DCO0;
	BCSCTL1 |= 0X08;

	//Set up SPI mode:
	UCA0CTL1 |= UCSWRST;
	UCA0CTL0 |= UCCKPL + UCMSB + UCMST + UCSYNC;
	UCA0CTL1 |= UCSSEL0 + UCSSEL1;	//Source USCI clock from SMCLK
	UCA0BR0 |= 0x08;		//Prescale the clock by 8

	UCA0CTL1 &= ~UCSWRST;

	//Set P1.1 as SOMI, P1.2 as SIMO, P1.4 as CLK
	P1SEL |= BIT1 + BIT2 + BIT4;
	P1SEL2 |= BIT1 + BIT2 + BIT4;

	//IE2 |= UCA0RXIE + UCA0TXIE;
	IE2 |= UCA0RXIE;
	_bis_SR_register(GIE);
	while(!(IFG2 & UCA0TXIFG)){}
	//SPI is ready to go!

	//Turn on SD Card
	P1OUT |= PWR;

	//1. Wait >1ms when SD card power reaches 2.2v --> @1.6MHz, >1600 ticks (10x that sounds good)
	int i=16000;
	while(i!=0){i--;}
	//2. Set SD card clock rate to 100kHz < SD CLK < 400kHz (see earlier)
	//3. Set CS=1, SIMO=1
	//4. Clock for >74 pulses (80 pulses or SPI transfers)
	i=0;
	for(i;i<10;i++){
		sendByte(0xFF);
	}
	//--------SD CARD IS IN POWER ON COMPLETE STATE---







	//PERFORM SD CARD SW RESET
	//1. Clear CS - Enable SPI to SD Card.
	P1OUT &= ~SDCS;
	//2. Send a Command frame with CMD0, CRC 0x95, arg 0
	sendFrame(0,0);
	//3. Check error register. Flag on the first bit means "Idle Mode" which we want
	if(errorCode==0x01){
		P1OUT |= GOOD;
	}





	for(;;) {

	}

	return 0;
}
/*
#pragma vector = USCIAB0TX_VECTOR
__interrupt void USCI0TX_ISR(void)
{
	P1OUT|= 0X01;
}
*/
#pragma vector = USCIAB0RX_VECTOR
__interrupt void USCIAB0RX_ISR(void)
{
	errorCode=UCA0RXBUF;
}
