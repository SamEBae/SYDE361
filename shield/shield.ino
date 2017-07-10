#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);
// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5
// read the buttons
int read_LCD_buttons()
{
 adc_key_in = analogRead(0);      // read the value from the sensor 
 // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
 // we add approx 50 to those values and check to see if we are close
 if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
 // For V1.1 us this threshold
 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 250)  return btnUP; 
 if (adc_key_in < 450)  return btnDOWN; 
 if (adc_key_in < 650)  return btnLEFT; 
 if (adc_key_in < 850)  return btnSELECT;  

 // For V1.0 comment the other threshold and use the one below:
/*
 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 195)  return btnUP; 
 if (adc_key_in < 380)  return btnDOWN; 
 if (adc_key_in < 555)  return btnLEFT; 
 if (adc_key_in < 790)  return btnSELECT;   
*/
 return btnNONE;  // when all others fail, return this...
}
int mode = 0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  lcd.begin(16, 2);              // start the library
  lcd.setCursor(8,0);
  lcd.print("|"); // print a simple message
  lcd.setCursor(0,0);
  lcd.print("CONT"); // print a simple message
}
int perfect = 119; //from observation, might need calibration

void loop() {
  // put your main code here, to run repeatedly:
  if (read_LCD_buttons() == 1) {
    lcd.setCursor(0,0);
    lcd.print("CONT"); // print a simple message
    mode = 0;
  }
  else if (read_LCD_buttons() == 2) {
    lcd.setCursor(0,0);
    lcd.print("DISC"); // print a simple message
    mode = 1;
  }
  if (analogRead(2) > 200) {
    Serial.println(analogRead(1));
    int input = analogRead(1)+(128-perfect);
    if (input > 255) input = 255;
    if (input < 0) input = 0;
    int value = (input)/16;
    if (mode == 0) {
      lcd.setCursor(value,1);
      lcd.print("|");
      for (int i = 0 ; i < 16 ; i += 1) {
        if (i != value){
          lcd.setCursor(i,1);
          lcd.print(" ");
        }
      }
    }
    if (mode == 1) {
      if (value > 8) {
        lcd.setCursor(13,1);
        lcd.print("<--");
        for (int i = 0 ; i < 13 ; i += 1) {
          if (i != value){
            lcd.setCursor(i,1);
            lcd.print(" ");
          }
        }
      }
      else if (value < 8) {
        lcd.setCursor(0,1);
        lcd.print("-->");
        for (int i = 3 ; i < 16 ; i += 1) {
          if (i != value){
            lcd.setCursor(i,1);
            lcd.print(" ");
          }
        }
      }
      else {
        lcd.setCursor(8,1);
        lcd.print("X");
        for (int i = 0 ; i < 16 ; i += 1) {
          if (i != 8){
            lcd.setCursor(i,1);
            lcd.print(" ");
          }
        }
      }
    }
  }
}