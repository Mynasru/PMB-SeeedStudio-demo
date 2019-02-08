/*----------------------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------//
//      _____  __  __ ____     _____                    _     _             _ _             _                           //
//     |  __ \|  \/  |  _ \   / ____|                  | |   | |           | (_)           | |                          //
//     | |__) | \  / | |_) | | (___   ___  ___  ___  __| |___| |_ _   _  __| |_  ___     __| | ___ _ __ ___   ___       //
//     |  ___/| |\/| |  _ <   \___ \ / _ \/ _ \/ _ \/ _` / __| __| | | |/ _` | |/ _ \   / _` |/ _ \ '_ ` _ \ / _ \      //
//     | |    | |  | | |_) |  ____) |  __/  __/  __/ (_| \__ \ |_| |_| | (_| | | (_) | | (_| |  __/ | | | | | (_) |     //
//     |_|    |_|  |_|____/  |_____/ \___|\___|\___|\__,_|___/\__|\__,_|\__,_|_|\___/   \__,_|\___|_| |_| |_|\___/      //
//                                                                                                                      //
//----------------------------------------------------------------------------------------------------------------------//                                                                                                         
//                                                                                                                      //
//     By:  Ruben Baldewsing                                                                                            //
//                                                                                                                      //
//      Version history:                                                                                                //
//        09/02/2019 - V2.2                                                                                             //
//                    * Fixed bugs in NeoPixel code                                                                     //
//        09/02/2019 - V2.1                                                                                             //
//                    * Fixes                                                                                           //
//        08/02/2019 - V2.0                                                                                             //
//                    * New hardware                                                                                    //
//        07/02/2019 - V1.0                                                                                             //
//                    * Initial test                                                                                    //
//                                                                                                                      //
//----------------------------------------------------------------------------------------------------------------------//
//----------------------------------------------------------------------------------------------------------------------*/

#define VERSION "2.2"

#define NUMBER_OF_PARTS 8                           //Number of parts connected

//Include libraries, more info: https://www.arduino.cc/reference/en/language/structure/further-syntax/include/
#include <Arduino.h>                                //Arduino abstraction layer (only for platformio)
#include <Wire.h>                                   //I2C serial bus library, more info: https://www.arduino.cc/en/Reference/Wire
#include <Adafruit_NeoPixel.h>                      //Adafruit Neopixel library, more info: https://learn.adafruit.com/adafruit-neopixel-uberguide
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

//Pin definitions, more info: https://www.arduino.cc/reference/en/language/structure/further-syntax/define/
//Inputs:
#define POTMETER_PIN A0
#define LIGHT_SENSOR_PIN A1
#define BUTTON_1_PIN 2                              //This pin can be used as hardware intterupt pin on ATMEGA328 based boards
#define BUTTON_2_PIN A3
#define TOUCH_BUTTON_PIN 4
#define IR_REFLECTIVE_SENSOR_PIN A2
//Outputs:
#define LED_SOCKET_KIT_PIN 3                        //PWM pin
#define NEOPIXEL_PIN 8
#define BUZZER_PIN 6                                //PWM pin
#define VIBRATION_MOTOR_PIN 7
#define MOSFET_PIN 5

//NeoPixel settings
#define NUM_LEDS 10
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, NEOPIXEL_PIN, NEO_GRBW + NEO_KHZ800);
uint32_t ACTIVE_SENSOR_COLOR = strip.Color(0, 255, 0, 20);
uint32_t SELECTED_SENSOR_COLOR = strip.Color(0, 0, 0, 200);

//OLED settings (128x32 pixels, auto reset)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Global variables
//Boolean
volatile boolean button_1_state = false;
boolean button_2_state = false;
boolean touch_state = false;
boolean ir_reflective_sensor_state = false;
boolean state = false;
volatile boolean start_loop = false;
volatile boolean auto_demo = false;
// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previous_millis = 0;                  //will store last time the demo was updated
volatile unsigned long current_millis = 0;                   //will store current time the demo was updated
volatile unsigned long button_1_not_pressed_for_millis =0;   //will store time button 1 was not pressed for
// constants won't change:
const long INTERVAL = 150;                          //Interval (ms) at which to update the the demo
const int AUTO_DEMO_INTERVAL = 5000;                //Automatically switch to next part in this interval (auto demo mode)
const int AUTOSTART_INTERVAL = 120;                  //number of seconds before the demo goes to automatic mode
//Interger
volatile int mode = 0;
int autostart_countdown_divider = 1000/INTERVAL;               //number of seconds before the demo goes to automatic mode
int autostart_countdown = AUTOSTART_INTERVAL*autostart_countdown_divider;
//16 bit integer (analogRead(pin) returns a 10 bit value on atmega328 boards!)
int potmeter_value = 0;
int light_sensor_value = 0;

//Interrupt service routine (ISR). More info: https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/
void isr_button_1(){
  if(mode>=NUMBER_OF_PARTS){
    mode=0;
  }
  else{
    mode++;
  }
  button_1_not_pressed_for_millis = current_millis;
  auto_demo = false;
  start_loop = true;
}

//Reset all actuators to default
void default_actuators(int actuator){
  if(actuator != 0) digitalWrite(LED_SOCKET_KIT_PIN,LOW);
  if(actuator != 1)digitalWrite(MOSFET_PIN,LOW);
  if(actuator != 2)digitalWrite(BUZZER_PIN,LOW);
  if(actuator != 3)digitalWrite(VIBRATION_MOTOR_PIN,LOW);
}
// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos){
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3,0);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3,0);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0,0);
}

//Read all analog ports
void analog_read_all(){
  potmeter_value = analogRead(POTMETER_PIN);
  light_sensor_value = analogRead(LIGHT_SENSOR_PIN);
}

void digital_read_all(){
  //button_1_state = digitalRead(BUTTON_1_PIN);
  button_2_state = digitalRead(BUTTON_2_PIN);
  touch_state = digitalRead(TOUCH_BUTTON_PIN);
  ir_reflective_sensor_state = digitalRead(IR_REFLECTIVE_SENSOR_PIN);
}

//Switch trough demo
void demo(){
  state =! state;                                   //Reverse this boolean value every time "demo()" is called
  strip.clear();                                    //Clear NeoPixel strip
  //strip.show();
  
  //Switch case function to select sensor/actuator. More info: https://www.arduino.cc/reference/en/language/structure/control-structure/switchcase/
  switch(mode){
    //Actuators
    case 0:                                         //LED socket kit
      digitalWrite(LED_SOCKET_KIT_PIN, state);
      default_actuators(mode);                      //Reset actuators to default exept this one
      strip.setPixelColor(0, SELECTED_SENSOR_COLOR);

      //Display info on the OLED
      display.println(F("LED SOCKET (RED)"));
      display.drawLine(0, 10, display.width(), 10, WHITE);
      display.setCursor(0,12);
      display.println(F("Adjust LED current\nwith the potmeter.\n- Digital\n- PWM\n- 3.3v/5v"));
      break;
    case 1:                                         //MOSFET
      digitalWrite(MOSFET_PIN, state);
      default_actuators(mode);                      //Reset actuators to default exept this one
      strip.setPixelColor(1, SELECTED_SENSOR_COLOR);

      //Display info on the OLED
      display.println(F("MOSFET"));
      display.drawLine(0, 10, display.width(), 10, WHITE);
      display.setCursor(0,12);
      display.println(F("Controll high current\nloads.\n- Digital\n- PWM\n- 5v\n- max. 15v"));
      break;
    case 2:                                         //Buzzer
      if(state) analogWrite(BUZZER_PIN, 128);
      else analogWrite(BUZZER_PIN, 0);
      default_actuators(mode);                      //Reset actuators to default exept this one
      strip.setPixelColor(2, SELECTED_SENSOR_COLOR);

      //Display info on the OLED
      display.println(F("BUZZER"));
      display.drawLine(0, 10, display.width(), 10, WHITE);
      display.setCursor(0,12);
      display.println(F("Create annoying beeps\n- Digital\n- PWM\n- 3.3v/5v\n- 300-2300Hz"));
      break;
    case 3:                                         //Vibration motor
      digitalWrite(VIBRATION_MOTOR_PIN, state);
      default_actuators(mode);                      //Reset actuators to default exept this one
      strip.setPixelColor(3, SELECTED_SENSOR_COLOR);

      //Display info on the OLED
      display.println(F("VIBRATION MOTOR"));
      display.drawLine(0, 10, display.width(), 10, WHITE);
      display.setCursor(0,12);
      display.println(F("Give your project\nsome vibe.\n- Digital\n- PWM\n- 5v"));
      break;
    //Digital sensors
    case 4:                                         //IR reflective sensor
      default_actuators(mode);                      //Reset all actuators to default
      if(ir_reflective_sensor_state == LOW){
        strip.setPixelColor(4, ACTIVE_SENSOR_COLOR);
      }
      else{
        strip.setPixelColor(4, SELECTED_SENSOR_COLOR);
      }

      //Display info on the OLED
      display.println(F("IR REFLECTIVE SENSOR"));
      display.drawLine(0, 10, display.width(), 10, WHITE);
      display.setCursor(0,12);
      display.println(F("Activates with a\nreflective surface.\n- Digital\n- 5v\n- 0-4.5mm"));
      break;
    case 5:                                         //Touch sensor
      if(touch_state == HIGH){
        strip.setPixelColor(5, ACTIVE_SENSOR_COLOR);
      }
      else{
        strip.setPixelColor(5, SELECTED_SENSOR_COLOR);
      }

      //Display info on the OLED
      display.println(F("TOUCH SENSOR"));
      display.drawLine(0, 10, display.width(), 10, WHITE);
      display.setCursor(0,12);
      display.println(F("Capacitive momentary\nswitch.\n- Digital\n- 2-5v\n- 60-220ms\n  response time"));
      break;
    case 6:                                         //Button 2
      if(button_2_state == HIGH){
        strip.setPixelColor(6, ACTIVE_SENSOR_COLOR);
      }
      else{
        strip.setPixelColor(6, SELECTED_SENSOR_COLOR);
      }

      //Display info on the OLED
      display.println(F("BUTTON (2)"));
      display.drawLine(0, 10, display.width(), 10, WHITE);
      display.setCursor(0,12);
      display.println(F("Momentary button\nswitch.\n- Digital\n- TTL\n- Normal: LOW"));
      break;
    //Analog sensors
    case 7:                                         //Potmeter
      strip.setPixelColor(7, Wheel(map(potmeter_value, 0, 1024, 0, 255) & 255));
      
      //Display info on the OLED
      display.println(F("POTENTIOMETER (POT)"));
      display.drawLine(0, 10, display.width(), 10, WHITE);
      display.setCursor(0,12);
      display.println(F("Variable resistor as\nvoltage divider.\n- Analog\n- 0-Vin\n- 300Kohm\n- 300deg rotation"));
      break;
    case 8:                                         //Light sensor
      strip.setPixelColor(8, Wheel(map(light_sensor_value, 0, 1024, 0, 255) & 255));

      //Display info on the OLED
      display.println(F("LIGHT SENSOR"));
      display.drawLine(0, 10, display.width(), 10, WHITE);
      display.setCursor(0,12);
      display.println(F("\nPhototransistor +\nopamp.\n- Analog\n- 5v\n- max. 350lux"));
      break;
  }
  strip.show();
}

//Setup only runs once at startup, more info: https://www.arduino.cc/reference/en/language/structure/sketch/setup/
void setup() {
  //Set pin modes
  //Inputs:
  pinMode(BUTTON_1_PIN,INPUT);
  pinMode(BUTTON_2_PIN,INPUT);
  pinMode(TOUCH_BUTTON_PIN,INPUT);
  pinMode(IR_REFLECTIVE_SENSOR_PIN,INPUT);
  //Outputs
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_SOCKET_KIT_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(MOSFET_PIN, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(BUTTON_1_PIN), isr_button_1, RISING);

  //Start hardware components
  Wire.begin(); //Start I2C hardware in the arduino.
  Serial.begin(115200); //Start serial (rx/tx) harware in the arduino.

  //Start OLED display
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.clearDisplay();                           //Clear OLED display
  display.setTextSize(1);                           //Normal 1:1 pixel scale
  display.setTextColor(WHITE);                      //Draw white text
  display.setCursor(0,0);                           //Start at top-left corner

  //This initializes the NeoPixel library.
  strip.begin();
  strip.show();

  //Print some info via Serial monitor and OLED screen
  Serial.println("* PMB SeeedStudio demo *\n");
  Serial.println("By:\nRuben Baldewsing\n");
  display.println(F("PMB SeeedStudio demo"));
  display.drawLine(0, 10, display.width(), 10, WHITE);
  display.setCursor(0,12);
  display.println(F("\nBy: Ruben Baldewsing"));
  display.display();
  delay(1000);
  Serial.println("Version: ");
  Serial.println("" VERSION " - " __DATE__ ", " __TIME__);
  display.print(F("\nVersion: "));
  display.println(F(VERSION));
  display.println(F("\n" __DATE__ ", " __TIME__));
  display.display();
  delay(5000);
  Serial.println("\nPress button 1, or wait to start the automatic demo mode...");
  Serial.println("Automatic demo mode starts in:");

  //Make some rainbow
  for(int i=0; i<strip.numPixels(); i++){
    strip.setPixelColor(i, Wheel(map(i, 0, strip.numPixels(), 0, 255) & 255));
    strip.show();
    delay(250);
  }

  //Display intro and wait for button 1 to be pressed, or the auto demo to start
  while(auto_demo==false && start_loop==false){
    current_millis = millis();
    if (current_millis - previous_millis >= INTERVAL) {
      previous_millis = current_millis;

      autostart_countdown--;
      if(autostart_countdown <=0){
        auto_demo = true;
        start_loop = true;
        Serial.println("Automode started");
      }

      Serial.print(autostart_countdown/autostart_countdown_divider);
      Serial.print(" ");

      display.clearDisplay();
      display.setCursor(0,0);
      display.println(F("Press button 1,\nor wait to start the automatic demo mode.."));
      display.print(F("\n\nAuto mode in: "));
      display.print(autostart_countdown/autostart_countdown_divider);
      display.display();
      //Put OLED intro code + auto mode countdown here !!!!
    }
  }

  //Button was pressed, or automode was started. Going to loop();
  Serial.println("Loop started");
  //Clear NeoPixels
  delay(500);
  strip.clear();
  strip.show();

  //Reset mode to 0
  mode = 0;
}

//Loop runs forever, more info: https://www.arduino.cc/reference/en/language/structure/sketch/loop/
void loop() {
  current_millis = millis();

  if(current_millis - previous_millis >= INTERVAL && !auto_demo){
    // save the last time you blinked the LED
    previous_millis = current_millis;

    //Serial.println("Manual demo loop running");
    //Serial.println();

    display.clearDisplay();
    display.setCursor(0,0);
    demo();
    display.display();
  }
  else if(current_millis - previous_millis >= AUTO_DEMO_INTERVAL && auto_demo){
    previous_millis = current_millis;

    //Serial.println("Auto demo loop running");

    if(mode>=NUMBER_OF_PARTS){
      mode=0;
      state = false;
    }
    else{
      mode++;
      state = false;
    }

    display.clearDisplay();
    display.setCursor(0,0);
    demo();
    display.display();
  }
  //Else skip this code and go on

  if(current_millis - button_1_not_pressed_for_millis >= (AUTOSTART_INTERVAL*1000) && !auto_demo){
    auto_demo = true;
    Serial.println("Automode started");
  }

  //Read the sensors every loop (only possible because the sensors are hardware debounced, otherwise use 50+ms delay)
  //delay(50);
  analog_read_all();
  digital_read_all();
}