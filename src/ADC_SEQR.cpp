#include "Arduino.h"
#include "ADC_SEQR.h"

  
volatile uint16_t Adc_Seqr::global_ADCounts_Array[] = {0};
uint8_t Adc_Seqr::numChannels = 14;

/*SAM3X ADC_MR Register, refer to data sheet:43.7.2 ADC Mode Register 

	31 	|	30	|	29 28	|	 27 26 25 24
USEQ 		-	  TRANSFER 		  TRACKTIM

	23 	|	22	|	21 20	|	 19 18 17 16
  ANACH		– 	  SETTLING 		   STARTUP

|	15	14	13	12		11	10	9	8	|
				PRESCAL

	7	|	6	|	5	|	4	|	321	|	0
FREERUN 	FWUP  SLEEP   LOWRES  TRGSEL  TRGEN
*/

void Adc_Seqr::init(){
	pmc_enable_periph_clk(ID_ADC);   //power management controller told to turn on adc
	ADC->ADC_CR |= ADC_CR_SWRST; //reset the adc
	ADC->ADC_MR = 0; //reset register
	ADC->ADC_MR |= ADC_MR_USEQ
				|  ADC_MR_TRANSFER(1)
				|  ADC_MR_TRACKTIM(15)
				|  ADC_MR_SETTLING(0)
				|  ADC_MR_STARTUP(8)
				|  ADC_MR_PRESCAL(255)
				|  ADC_MR_TRANSFER(3);
	ADC->ADC_CHDR = MAX_FIELD;   // disable all channels
}

void Adc_Seqr::start(){
	numChannels = getSettedCh(); //set how namy channel will be on buffer

	ADC->ADC_SEQR1 = 0x01234567;  // use A0 to A7 in order into array 
	ADC->ADC_SEQR2 = 0x00fedcba;      //use A8 to A11, 52 and INTERNAL_TEMP following in order into array
	NVIC_EnableIRQ(ADC_IRQn); // interrupt controller set to enable adc.
	ADC->ADC_IDR = ~ADC_IDR_ENDRX; // interrupt disable register, disables all interrupts but ENDRX

	ADC->ADC_IER = ADC_IDR_ENDRX;   // interrupt enable register, enables only ENDRX
 
 // following are the DMA controller registers for this peripheral
 // "receive buffer address" 
	ADC->ADC_RPR = (uint32_t)global_ADCounts_Array;   // DMA receive pointer register  points to beginning of global_ADCount
	// "receive count" 
	ADC->ADC_RCR = numChannels;  //  receive counter set
	// "next-buffer address"
	ADC->ADC_RNPR = (uint32_t)global_ADCounts_Array; // next receive pointer register DMA global_ADCounts_Arrayfer  points to second set of data 
	// and "next count"
	ADC->ADC_RNCR = numChannels;   //  and next counter is set
	// "transmit control register"
	ADC->ADC_PTCR = ADC_PTCR_RXTEN;  // transfer control register for the DMA is set to enable receiver channel requests
	// now that all things are set up, it is safe to start the ADC.....
	ADC->ADC_MR |= ADC_MR_FREERUN;//0x80; // mode register of adc bit seven, free run, set to free running. starts ADC
	delay(100); //for stability
}


void Adc_Seqr::begin(){
	init();

	ADC->ADC_ACR |= ADC_ACR_TSON; // enable internal temp sensor
	ADC->ADC_CHER  |= ADC_CHER_CH0 
					| ADC_CHER_CH1 
					| ADC_CHER_CH2 
					| ADC_CHER_CH3 
					| ADC_CHER_CH4 
					| ADC_CHER_CH5 
					| ADC_CHER_CH6 
					| ADC_CHER_CH7
					| ADC_CHER_CH8 
					| ADC_CHER_CH9
					| ADC_CHER_CH10 
					| ADC_CHER_CH11 
					| ADC_CHER_CH12 
					| ADC_CHER_CH13; 
	start();
}

uint8_t Adc_Seqr::getSettedCh(){ //found how many channel are set
	uint8_t numChannels = 0;
	for (int i = 0; i < 16; i++){
		if ( ADC->ADC_CHSR & (1<<i) ) numChannels++;
	}
	return numChannels;
}

void Adc_Seqr::printSetup(){
	uint32_t adc_clk = 84000/ (  ( ( (ADC->ADC_MR & ADC_MR_PRESCAL(MAX_FIELD)>>8) +1) *2 ) );
	Serial.print("mode register =            "); Serial.println(REG_ADC_MR, HEX);
	Serial.print("channel enabled register = "); Serial.println(REG_ADC_CHSR, BIN);
	Serial.print("sequence register1 =       "); Serial.println(REG_ADC_SEQR1, HEX);
	Serial.print("sequence register2 =       "); Serial.println(REG_ADC_SEQR2, HEX);
	Serial.print("interrupts =               "); Serial.println(REG_ADC_IMR, HEX);
	Serial.print("ADC Clock KHz =            "); Serial.println(adc_clk);
	Serial.print("ADC sample rate KHz=       "); Serial.println(ADC_sampleRate()/1000);
}

uint16_t Adc_Seqr::read(uint8_t pin){
	switch (pin) { //handle 0-11 and A0 to A11 pin, if invalid return 0
		case 0:;
		case 1:;
		case 2:;
		case 3:;
		case 4:;
		case 5:;
		case 6:;
		case 7:;
		case 8:;
		case 9:;
		case 10:;
		case 11: break;
		case INTERNAL_TEMP: pin = INTERNAL_TEMP; break;
		case 52: pin = 12; break;
		case 54:;
		case 55:;
		case 56:;
		case 58:;
		case 59:;
		case 60:;
		case 61:;
		case 62:;
		case 63:;
		case 64:;
		case 65:;
		case 66 : pin-=A0; break;
		default : return -1;
	}

	uint8_t ch = pin;
	if( !(ADC->ADC_CHSR & (1<<ch)) ) {
		return 0; //return 0 for disable pin
	}

	uint8_t pos = getArrayPos(pin);	

	return global_ADCounts_Array[pos];
}

uint8_t Adc_Seqr::getArrayPos( uint8_t pin ) {
  uint8_t pos = 0;
  for (uint8_t i=0; i<pin; i++){  //loop add a position in buffer for each active pin in ADC_CHSR
    if ( ADC->ADC_CHSR & (1<<i) ) pos++; 
  }
  return pos; 
}


void Adc_Seqr::ADCHandler() {     // for the ATOD: re-initialize DMA pointers and count	
	
	//   read the interrupt status register 
	if (ADC->ADC_ISR & ADC_ISR_ENDRX){ /// check the bit "endrx"  in the status register /// ADC_IDR_ENDRX correction
		/// set up the "next pointer register" 
		ADC->ADC_RNPR =(uint32_t)global_ADCounts_Array;  // "receive next pointer" register set to global_ADCounts_Array 
		// set up the "next count"
		ADC->ADC_RNCR = numChannels;  // "receive next" counter
	}
}

void Adc_Seqr::prescaler(uint32_t prsc) {
	ADC->ADC_MR &= ~(ADC_MR_PRESCAL(MAX_FIELD));   //mode register "prescale" zeroed out. 
	ADC->ADC_MR |= ADC_MR_PRESCAL(prsc);  //setup the prescale frequency
}

void Adc_Seqr::setTracktim(uint8_t tracktim) {
	ADC->ADC_MR &= ~(ADC_MR_TRACKTIM(MAX_FIELD));
	ADC->ADC_MR |= ADC_MR_TRACKTIM(tracktim);
}

void Adc_Seqr::enable(){
	ADC->ADC_MR |= ADC_MR_FREERUN;
}

void Adc_Seqr::disable(){
	ADC->ADC_MR &= ~ADC_MR_FREERUN;
}

  //interrupt handler
void ADC_Handler(){
    Adc_Seqr::ADCHandler();
}

uint32_t Adc_Seqr::ADC_sampleRate(){
	uint32_t f2 = 84000000/( ( ( (ADC->ADC_MR &  ADC_MR_PRESCAL(MAX_FIELD)) >>8) +1) *2 );
	f2 /= CLOCK_CYCLE_PER_CONVERSION;
	return f2 / getSettedCh();
}


float internalTemp() { // convert the sam3x internal sensor temp
	if ( !(ADC->ADC_CHSR & 1<<13) ) return 0;  // return 0 if internal temps ch is disable
	float l_vol = Adc_Seqr::read(INTERNAL_TEMP) * 3300 / 4095;
	return (float)(l_vol-800) * 0.37736 +25.5; //  <-- the last ofset can e calirated  TODO: find method...
}
