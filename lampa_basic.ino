/*
TODO
*/
//#include "math.h"
//hardware  definitions
const int RED = P2_4;
const int BLUE = P2_2;
const int GREEN = P2_1;
const int POT = A4;

//processing definitions
long int POT_Zero = 0;
const int POT_res = 50; 
const int RED_max = 147; // int (1.9 V / 3.3 V * 255)

const int samples = 1024;   //length for POT sampling


int red, green, blue;

//function signatures
void set_LED(int red, int green, int blue);
long int samplePOT();
void change_mode_func();

long int oldVal = 0;
long int delta;
// the setup routine runs once when you press reset:
void setup() {
    Serial.begin(9600);
    delay(1000);
}


// the loop routine runs over and over again forever:
void loop() {
  long int val = samplePOT();
  delta = val - oldVal;
  //check if let go
  if ( delta < -oldVal / 2){
    //Serial.print("true ");
    val = 0;
  }

 
  
  //  Serial.print("old "); Serial.print(oldVal); Serial.print( "; new ");

  oldVal = val;  
  Serial.println(val);

  if (val > POT_res){// if input from the pot
      
      //compute colour
      if (val < 512){
          green = 255 - val / 2;
          red = val / 2;
          blue = 0;
      }
      else{
          val -= 512;
          green = 0;
          red = 255 - val/2;
          blue = val / 2;
      }
      
      
      //set LED to colour
      set_LED(red, green, blue);
  }
  
  delay(1);
 
 }
 
 void set_LED(int red, int green, int blue){
   // set an RGB LED to a colour
   analogWrite(RED, red * RED_max / 255);
   analogWrite(GREEN, green);
   analogWrite(BLUE, blue);  
 };
 
long int samplePOT (){
     long int sum = 0;
     for(int i=0; i < samples; ++i){
         //readings[i] = analogRead(POT);
         sum += analogRead(POT);
     }
     return sum / samples;
}

 
