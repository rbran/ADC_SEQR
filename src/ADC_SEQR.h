#ifndef ADC_SEQR_h
#define ADC_SEQR_h

#if defined(ARDUINO_ARCH_AVR)
  	#error "This libraries is for arduino DUE only !!!"
#elif defined(ARDUINO_ARCH_SAM)
  // SAM-specific code


#define INTERNAL_TEMP 13

class Adc_Seqr;


								//PIN 52 (AD14)    							 Shift 2 bit for the 2 adc channel (8,9)not use in samx       enable internal temp sensor
static void enableChX(uint8_t pin){ if(pin==52) pin==14; else if(pin>=A0) pin-=A0; else if (pin>7 && pin!=14) pin+=2; if(pin>15) return; if(pin==INTERNAL_TEMP+2) ADC->ADC_ACR |= ADC_ACR_TSON; ADC->ADC_CHER|=(1<<pin); };
														//shift analog pin to int 								ignore input bigger than reggister

template<typename Pin, typename ... PinX>
static void enableChX(Pin pin, PinX ... pinX) { enableChX(pin); enableChX(pinX...); };


  
class Adc_Seqr  {
public:
	static void begin();
	template<typename ... PinX>
	static void begin(PinX ... pinX){ init(); enableChX(pinX...); start(); };
	static void ADCHandler();
	static uint16_t read(uint8_t pin);
	static void prescaler(uint32_t prsc);
	static void printSetup();
	static void enable();
	static void disable();

	private:

	static uint8_t NUM_CHANNELS; //number of analog channel active
	const static uint8_t MAX_NUM_CHANNELS = 14;
	static volatile uint16_t global_ADCounts_Array[MAX_NUM_CHANNELS];  // holds the raw data from the analog to digital	
		
	static void init();
	static void start();

	static uint8_t getArrayPos( uint8_t pin = 0 );
	static uint8_t getSettedCh();
};

float internalTemp();

#else
  #error "No SAM3x (arduino DUE) achitechture detected !!!"
#endif
	
#endif


