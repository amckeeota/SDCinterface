//JUST FINISHED UP TO GETTING PAST IDLE STATE (tested on SDC ver 1)
//CHANGED: Added simple delays until UCA0RXIFG vs interrupts for better control
//NEED TO: communicate with SDHC - get response to be correct / by the time I send cmd 55 for SDHC, it is off by one nibble


#include <msp430g2553.h>

//Pin Defines
#define SDCS 0x08
#define SIMO 0x04
#define SOMI 0x02
#define CLK 0x10
#define GOOD 0x40
#define BAD 0x01
#define PWR 0x20


unsigned char ultimateData [100];
//__interrupt void USCIAB0RX_ISR (void);

unsigned char sendByte(unsigned char txByte){
	UCA0TXBUF=txByte;
	while(!(IFG2 & UCA0TXIFG)){}
	while(IFG2 & UCA0RXIFG){
		unsigned char j = UCA0RXBUF;
	}
		return UCA0RXBUF;
}

void readBlock(){
	while(sendByte(0xff)!=0xfe){}
	int i;
	for(i=0;i<100;i++){
		ultimateData[i] = sendByte(0xff);
	}
	for(i=0;i<415;i++){
		sendByte(0xff);
	}
/*	for(i=0;i<100;i++){
		ultimateData[i] = sendByte(0xff);
	}
	for(i=0;i<100;i++){
		ultimateData[i] = sendByte(0xff);
	}
	for(i=0;i<100;i++){
		ultimateData[i] = sendByte(0xff);
	}
	for(i=0;i<15;i++){
		ultimateData[i] = sendByte(0xff);
	}*/
}

unsigned long sendFrame(unsigned int sdcmd, long cmdArg, int CRC, int responseBytes){
	P1OUT &= ~SDCS;
	sendByte(0x40+sdcmd);		//Frame up the command
	sendByte(cmdArg>>24);	//Send the four byte argument, MSB fist
	sendByte(cmdArg>>16);
	sendByte(cmdArg>>8);
	sendByte(cmdArg);
	sendByte(CRC);			//Send the CRC command (only important for SDC SW Reset)
	//Send 0xff on the TX for NCR frames/until we get a response from the SDC!
	unsigned int timeout=0;
	unsigned char errorCode = 0xFF;
	while(errorCode == 0xFF){
		errorCode = sendByte(0xFF);
		timeout++;
		if(timeout==16){
			P1OUT |= BAD;
			break;
		}
	}
	//at this point, errorCode is loaded with R1

	if(errorCode > 1){
		P1OUT |= SDCS;
		return 5;
	}
	unsigned long response = (unsigned long)errorCode;


	if(sdcmd == 17 && errorCode == 0x00){
		readBlock();
	}


	int i;
	for(i=0;i<responseBytes;i++){
		response = response << 8;
		sendByte(0xFF);
		response |= UCA0RXBUF;
	}

	P1OUT |= SDCS;
	return response;
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
	//IE2 |= UCA0RXIE;
	//_bis_SR_register(GIE);
	while(!(IFG2 & UCA0TXIFG)){}
	//SPI is ready to go!

	//Turn on SD Card
	int i=32000;
	while(i>0){i--;}
	P1OUT |= PWR;

	//1. Wait >1ms when SD card power reaches 2.2v --> @1.6MHz, >1600 ticks (10x that sounds good)
	i=16000;
	while(i!=0){i--;}
	//2. Set SD card clock rate to 100kHz < SD CLK < 400kHz (see earlier)
	//3. Set CS=1, SIMO=1
	//4. Clock for >74 pulses (80 pulses or SPI transfers)
	i=0;
	for(i;i<10;i++){
		sendByte(0xFF);
	}

	//Put SD Card into idle state
	unsigned long sdResponse = 0;
	sdResponse = sendFrame(0,0,0x95,0);

	//Check OCR in order (determines which version of SD Card to use)
	sdResponse = sendFrame(8,0x1AA,0x87,4);	//Check SD Card Working conditions for > SD Ver 1
	if(sdResponse == 0x05){
		sdResponse = sendFrame(58,0,0x75,4);	//Executed only for SD Ver 1 - check operating conditions
		sdResponse = 0x01;
		//Initialize SD Card Ver 1
		while(sdResponse != 0){
			sdResponse = sendFrame(55,0,0x95,0);	//Send ACMD41 until we get out of idle state
			sdResponse = sendFrame(41,0,0x95,0);
			if(sdResponse == 0){
				P1OUT |= GOOD;
			}
		}
	}
	else{
		//Initialize SD Card Ver 2
		while(sdResponse != 0){
			sdResponse = sendFrame(55,0,0x95,0);	//Send ACMD41 until we get out of idle state
			sdResponse = sendFrame(41,0x40000000,0x95,0);
			if(sdResponse == 0)
			{
				P1OUT |= GOOD;
			}
		}
	}

	if(sendFrame(58,0,0x95,4) & 0x40000000){}
	else{
		sdResponse = sendFrame(16,0x200,0x95,0);	//change a block size to 512 Bytes
	}

	//CLean this part up: reading
		sdResponse = sendFrame(17,49294,0x95,0);

	for(;;) {

	}

	return 0;
}
