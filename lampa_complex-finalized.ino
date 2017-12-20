/*
 * Changing colours lamp based on
 * MSP-EXP430G2553LP board
 * 
 * Hardware: 
 * Red    1.9 V LED @ pin P2.4
 * Green  3.3 V LED @ pin 2.1
 * Blue   3.3 V LED @ pin P2.2
 * Strip potentiometer (10kOm) @ pin A4
 * NB: Potentiometer needs to be grounded trough chip to mitigate noise
 * 
 * Desired behaviour:
 * 
*/
//#include "math.h"
//hardware  definitions -pins
const int RED = P2_4;
const int GREEN = P2_1;
const int BLUE = P2_2;
const int POT = A4;

//processing definitions
const int POT_zero = 0;   //bottom threshold
const int POT_max = 1023; //max value
const int POT_res = 50;   //resolution
const int RED_max = 147;  //int (1.9 V / 3.3 V * 255)

const int samples = 64;   //length for POT sampling

const float delta_let_go_fac = 0.5 //empirically chosen value for the signal derivative
//used to detect the steep slope corresponding to the user letting go of the strip

//change mode definitions
const unsigned long int change_mode_time = 1000;//how long to press to get into change mode
int change_mode_start_val = 0;                  //record of the value when holding pressed
unsigned long int change_mode_start_t;          //record of when holding pressed started
bool change_mode_holding = 0;                   //flag 
bool turn_off = 0;                              //flag

//function signatures
void set_LED(unsigned char red, unsigned char green, unsigned char blue);
int samplePOT();
void change_mode_func(int val);
void standard_mode_func(int val);
int sign(int x);
/*unsigned long int fade_func(int val){
  return val*val/20000;
}*/



// the setup routine runs once when you press reset:
void setup() {
    Serial.begin(9600); //use baud rate 9600
    delay(1000);        //delay before execution for stability
}

//intitialize loop variables
int oldVal = 0;
int delta;

// the loop routine runs over and over again forever:
void loop() {
  /*
   * Three options: 
   * 
   * standard mode - user tapped to select a single colour
   * 
   * change mode - user held the input to get into a 'disco mode'
   *               the colour changes randomly with period dependent on the input
   *               
   * turn off - user held the end of the strip pressed to turn the light off
   */


  int val = samplePOT();  //current averaged value of the potentiometer
  delta = val - oldVal;   //change in the potentiometer value since previous loop
  
  //check if the user has let go of pressing the strip
  if ( delta < -oldVal * delta_let_go_fac){
    //detect sudden signal drop 
    //Serial.print("You let go");
    val = POT_zero; //reset to zero to avoid oversampling and going to mid-level
  }
  oldVal = val;
  //Serial.println(val);
  if (val > POT_res){ //if input from POT is bigger than noise threshold
    
    if (change_mode_holding){ // if the user has been holding for a while
       //keep the delta with respect to the start of the change mode to avoid drift
       delta = val - change_mode_start_val;   
    }
    
    if (abs(delta) < POT_res){ //if button is being held
        
        if (!change_mode_holding) {
          //the first time we detect the user is keeping the strip pressed
          //Serial.println("starting to hold");
          change_mode_holding = 1;
          change_mode_start_t = millis(); //when start holding
          change_mode_start_val = oldVal; //at what value holding
        }

        if(millis() - change_mode_start_t > change_mode_time){
          //start executing change mode after some time
          //Serial.println("starting change mode");
          change_mode_func(change_mode_start_val); //should interrupt on button press
        }

        //Serial.println("holding...");
    } //if button is being held
    else{ //when the user lets go of the pot
      change_mode_holding =0;
    }
    
    if (turn_off){ //turn off is global, can be set to zero from change_mode_func
      //Serial.println("seriously, shut up");
      set_LED(0,0,0);
      return;
    }
    
    //most of the time, on single tap input
    standard_mode_func(val);  
  
  } //if input from POT
  else if (turn_off){ //just in case
    turn_off = 0;  
  }
   
  delay(1);
 
} //loop

void standard_mode_func(int val){
  /*
   * Set the colour of the light based on the location of the tap on the strip
   */
  unsigned char red, green, blue;
  //compute colour - equally spaced, red in the middle
  if (val < 512){
    //if left of the middle - combine red and green
          green = 255 - val / 2;
          red = val / 2;
          blue = 0;
  }
  else{
    //if right of the middle - combine red and blue
          val -= 512;
          green = 0;
          red = 255 - val/2;
          blue = val / 2;
  }
      
  //set LED to colour
  set_LED(red, green, blue);
    
}

void change_mode_func (int start_val){
  // loops while in change mode
  /*
   * Randomly change colours at a different rate
   * 
   * start_val - the POT input where the user is holding it pressed
   */
  
  //intialization variables
    //flags
  bool change_mode_running = 1;   //is change mode active
    //disco details
  unsigned long int fade_period = start_val/100; //period between colour changes
  unsigned long int fade_last_time = millis();   //last time there was a colour change?
  int fade_direction;   //make bluer or greener
  int target_val = val; //final colour of current period
    //init
  int val = start_val;
  //Serial.println("starting to change mode");
  
  while (change_mode_running){
    //run loop while change mode is active instead of the main 'loop' function
    
    //Serial.println("inside change mode");
    int POT_val = samplePOT();

  //----------------exceptional cases---------------------------------
    if (!(abs(POT_val-start_val) < POT_res && change_mode_holding)){
      //don't do this bit if user is still holding to get into the change mode
      change_mode_holding = 0; //the user is not holding anymore
      if (turn_off){ //turn off signal is detected later in the code
        //turn off if turned off
        change_mode_running = 0;  
      }
    }

    if (!change_mode_holding && POT_val > POT_res){
      //exit change mode if user presses anywhere after initialization 
      //Serial.println("exiting change mode");
      change_mode_running = 0;  //set flag
    }

    if (start_val > POT_max - 2*POT_res){ //when very close to the maximum
        //Detect turn off signal when the end  is pressed
        //Serial.println("turning off");
        set_LED(0,0,0); //turn off light
        turn_off = 1;   //set flag
    }
  //-------------------------------------------------------------------

  //-------------standard cases----------------------------------------
    if (val == target_val){
      //at the start & every time it reaches the preset point
      //Serial.println("long change");
      target_val = random(1023); //choose new target
      fade_direction = sign(target_val - val); //calculate whether new target is bluer or greener
    }

    if (millis() > fade_last_time){
      //wait at least one fade period before incrementing
      //Serial.println("short change");
      fade_last_time += fade_period;//reset time for next increment
      val += fade_direction;        //increment the colour
      standard_mode_func(val);      //use standard function to set the colour
    }

    //Serial.println(val);
    delay(1); //for stability
  //-------------------------------------------------------------------
  
  } // while change_mode_running
 } //change_mode_func

 
 void set_LED(unsigned char red,unsigned char green,unsigned char blue){
   // set an RGB LED to a colour for convenience
   analogWrite(RED, red * RED_max / 255);
   analogWrite(GREEN, green);
   analogWrite(BLUE, blue);  
 } //set_LED
 
int samplePOT (){
    // average of a number of samples - filter necessary for stability
     long int sum = 0;
     for(int i=0; i < samples; ++i){
         //readings[i] = analogRead(POT);
         sum += analogRead(POT);
     }
     return sum / samples;
} //sample pot

int sign(int x) {
  //convenience function 
    return (x > 0) - (x < 0);
}
