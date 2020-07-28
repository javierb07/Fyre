/*
 FILENAME:      KickJrPPG.cpp
 AUTHOR:        Orlando S. Hoilett
 EMAIL:			orlandohoilett@gmail.com
 VERSION:		1.0.0
 
 
 ABBREVIATIONS
 
 PPG - Photoplethysmography - Process of measuring changes in volume in light.
	Used to measure heart rate, respiration, and blood oxygen by measuring
 	changes in volume in the blood vessels.
 
 BPF - Bandpass filters
 
 I2C
 
 
 DESCRIPTION
 
 
 A FEW RESOURCES:
 
 Analog-to-Digital Conversion
 https://www.instructables.com/id/Analog-to-Digital-Conversion-Tutorial/
 
 I2C
 
 Digital Potentiometers
 
 
 UPDATES
 
 
 */


#include "KickJrPPG.h"


const uint8_t ledPin = 30;
const uint8_t ppgPin = A4;
const uint8_t tiaPin = A5;
const uint8_t ad5171_address = 0x2D;
const uint8_t ad5242_address = 0x2C;



//DEFAULT CONSTRUCTOR
//Must be called before setup in .ino code
KickJrPPG::KickJrPPG()
{
}


//void KickJrPPG::begin()
//
//Initializes functions, member variables, and member objects of the PPG class
//Must be called before setup in .ino code
void KickJrPPG::begin()
{
	pinMode(ledPin, OUTPUT);
	turnLEDOff();
	
	analogReadResolution(12); //12-bits = 0-4095
	
	setLEDCurrent(63); //smallest LED current by default

	ad5242 = AD524X(ad5242_address);
}


//void KickJrPPG::turnLEDOn()
//Turns on LED by writing HIGH to pin. LED is driven using an NPN bipolar
//junction transistor (BJT).
void KickJrPPG::turnLEDOn()
{
	digitalWrite(ledPin, HIGH);
	ledState = true;
}


//void KickJrPPG::turnLEDOff()
//Turns off LED by writing LOW to pin. LED is driven using an NPN bipolar
//junction transistor (BJT).
void KickJrPPG::turnLEDOff()
{
	digitalWrite(ledPin, LOW);
	ledState = false;
}


//bool KickJrPPG::getLEDStatus() const
//Returns whether or not the LED is on (true) or off (false).
bool KickJrPPG::getLEDStatus() const
{
	return ledState;
}


//void KickJrPPG::setLEDCurrent(uint8_t val)
//val		value to write to AD5171. Must be between 0 and 63.
//
//Code Reference: https://www.arduino.cc/en/Tutorial/DigitalPotentiometer
//
//This function writes to the digital potentiometer (AD5171 in this version)
//that controls the brightness of the LED.
//
//Current into the LED is determined by the following equations:
//		I_B = (3.3V-0.6V)/(R_pot + R16)
//		LED_current = I_B*Gain
//		Current gain of an NPN transistor is
//		often written as HFE in datasheets
//
//"val" has an inverse relationship with LED current
//As val goes up, resistacee goes up, LED current goes down
//As val goes down, resistacee goes down, LED current goes up
void KickJrPPG::setLEDCurrent(uint8_t val)
{
	if(val > 63) val = 63;
	
	Wire.beginTransmission(ad5171_address); //address is specified in datasheet
	Wire.write(byte(0x00)); // sends instruction byte
	Wire.write(val); // sends potentiometer value byte (between 0 and 63)
	Wire.endTransmission(); // stop transmitting
	
	R_ledCurrent = val;
}


//uint8_t KickJrPPG::getLEDCurrent() const
//
//Returns the setting of the digital potentiometer controlling the LED current.
uint8_t KickJrPPG::getLEDCurrent() const
{
	return R_ledCurrent;
}


//void KickJrPPG::balanceLEDCurrent()
//
//Balances the LED brightness with the TIA gain. Sets the TIA gain to a nominal
//value given the TIA maximum range. Setting the digital potentiomter (AD5242)
//to decimal = 17, gives a resistance of 67 kiloOhms. By doing so, the TIA gain
//can be moduled in at least one order of magnitude in either direction,
//smaller (6.7 kiloOhms) or larger (667 kiloOhms).
//
//Then proceeds to adjust the LED brightness around this nominal TIA gain.
void KickJrPPG::balanceLEDCurrent()
{
	//set to some nominal value
	setTIAGain(17);
	
	//add a bit of delay for settling time;
	delay(1);
	
	//measure TIA output voltage
	uint16_t Vtia = getTIA();
	
	//the bounds are about 10% of 1/2 of full ADC scale
	//adjust the LED current so the TIA fits these bounds
	while(Vtia > 2250 || Vtia < 1850)
	{
		//if LED is too bright (voltage output of the
		//TIA is too large, then decrease LED brightness
		if(Vtia > 2250 && R_ledCurrent > 0) setLEDCurrent(R_ledCurrent-1);
		
		//if LED is too dim (voltage output of the
		//TIA is too small, then increase LED brightness
		else if(Vtia < 1850 && R_ledCurrent < 63) setLEDCurrent(R_ledCurrent+1);
		
		//if I'm already at the minimum or maximum of the LED current, then
		//I need to adjust the TIA gain instead
		else balanceTIAGain();
		
		//add a bit of delay for settling time;
		delay(1);
		
		//check the TIA voltage again
		Vtia = getTIA();
	}
}


//void KickJrPPG::setTIAGain(uint8_t gain)
//gain			value between 0-255 to set resistance of digital potentiometer
//
//Sets gain of digital potentiometer connected to transimpedance amplifier.
//As gain increases, resistance of potentiometer increases, gain of TIA
//increases, output voltage increases.
void KickJrPPG::setTIAGain(uint8_t gain)
{
	ad5242.write(0,gain);
	R_tiaGain = gain;
}


//uint8_t KickJrPPG::getTIAGain() const
//
//Returns gain setting of digital potentiometer connected to TIA
uint8_t KickJrPPG::getTIAGain() const
{
	return R_tiaGain;
}


//void KickJrPPG::balanceTIAGain()
//
//Balances the TIA gain with the LED gain. Sets the TIA gain to 1/2 full
//ADC. Since the PPG circuit has a "virtual ground" at 1/2 full ADC,
//setting the output of the TIA gives the PPG circuit the lowest settling
//time and highest responsivity.
void KickJrPPG::balanceTIAGain()
{
	//measure TIA output voltage
	uint16_t Vtia = getTIA();
	
	while(Vtia > 2250 || Vtia < 1850)
	{
		//if voltage output of TIA is too large, then decrease TIA gain
		if(Vtia > 2250 && R_tiaGain > 0) setTIAGain(R_tiaGain-1);
		
		//if voltage output of TIA is too small, then increase TIA gain
		else if(Vtia < 1850 && R_tiaGain < 255) setTIAGain(R_tiaGain+1);
		
		//if R_tiaGain parameter is already at its maximum (255)
		//or minimum (0), then adjust the LED current instead.
		else balanceLEDCurrent();
		
		delay(1); //add a bit of delay for settling time;
		
		Vtia = getTIA();
	}
}


//void KickJrPPG::setPPGGain(uint8_t gain)
//gain			value between 0-255 to set resistance of digital potentiometer
//
//Sets gain of digital potentiometer connected to bandpass filter.
//As gain parameter increases, resistance of potentiometer increases, gain of
//bandpass filter increases, output voltage increases.
void KickJrPPG::setPPGGain(uint8_t gain)
{
	ad5242.write(1,gain);
	R_ppgGain = gain;
}


//uint8_t KickJrPPG::getPPGGain() const
//
//Returns gain setting of digital potentiometer connected to bandpass filter
uint8_t KickJrPPG::getPPGGain() const
{
	return R_ppgGain;
}


//uint16_t KickJrPPG::getTIA() const
//
//Reads voltage of transimpedance amplifier
uint16_t KickJrPPG::getTIA() const
{
	return analogRead(tiaPin);
}


//uint16_t KickJrPPG::getPPG() const
//
//Reads voltage of the bandpass filter of the PPG circuit.
uint16_t KickJrPPG::getPPG() const
{
	return analogRead(ppgPin);
}





