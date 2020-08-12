/* Fyre Smartwatch - Developed during summer 2020 in Purdue Milestones workshop
 * Javier Belmonte
 */

// Include necessary libraries
#include <KickJrPPG.h>
#include <Wire.h>

KickJrPPG ppg;  // Create an object of the KickJrPPG library

// 

void setup() {
  Serial.begin(9600); // Begin serial communication at 9600 bauds
  ppg.begin();  // Call begin method 
  Wire.begin(); // Join I2C bus (address optional for master)
}

void loop() {
  // Start sampling PPG circuit

}
