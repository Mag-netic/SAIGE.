


//Project Saige

//Libraries used
#include <Wire.h>              //Wire library for communicating through I2C
#include <Adafruit_PCF8574.h>  //Library for PCF8574 GPIO expander, opens up 8 GPIO pins through one address 0x20
#include <TFT_eSPI.h>
#include <debounce.h>
#include <animation.h>
//#include <Adafruit_ImageReader.h>
//#include <Adafruit_ImageReader_EPD.h>
//#include <SdFat_Adafruit_Fork.h>
//#include <Adafruit_GFX.h>
//#include <Adafruit_GrayOLED.h>
//#include <Adafruit_SPITFT.h>
//#include <Adafruit_SPITFT_Macros.h>
//#include <gfxfont.h>

#include <TJpg_Decoder.h>
#define FS_NO_GLOBALS
#include <FS.h>
#ifdef ESP32
#include "SPIFFS.h"  // ESP32 only
#endif
#define SD_CS 5
#include "SPI.h"

// The available pins on the CYD are
// GPIO 35 — on the P3 connector, GPIO 22 — on the P3 and CN1 connector, GPIO 27 — on the CN1 connector.
// GPIO 27 will be used for SERIAL DATA for I2C communication.
// GPIO 22 will be used for SERIAL CLOCK for I2C Communication.
#define I2C_SDA 27  //Sets I2C Data to GPIO pin 27.
#define I2C_SCL 22  //Sets I2c Clock to GPIO pin 22.

// #define BL_PIN 21 // On some cheap yellow display model, BL pin is 27
// #define SD_CS 5 //SD reader chip select pin
// #define SD_MISO 19 //SD reader Master In/Slave Out pin
// #define SD_MOSI 23 //SD reader Master out/Slave in pin
// #define SD_SCK 18 //SD reader clock pin
// #define SD_SPI_SPEED 80000000L      // 80Mhz

//Adafruit_SPITFT tft();
//SPIClass SDspi = SPIClass(VSPI);

#define PCF_BUTTON_A 6    //Initializes GPIO pin 6 of the PCF8574.
#define PCF_BUTTON_B 5    //Initializes GPIO pin 5 of the PCF8574.
#define PCF_BUTTON_C 4    //Initializes GPIO pin 4 of the PCF8574.
#define ADA_INTER_PIN 35  //Sets the PCF8574 interupt pin (INT) to GPIO pin 35.
#define STEP_PIN 0

#define SCREEN_WIDTH 320   //Sets the screen width at 320 pixels.
#define SCREEN_HEIGHT 240  //Sets the screen hight at 240 pixels.
#define FONT_SIZE 2        //Sets the font size.



hw_timer_t *stomachTimer = NULL;  //declared timer
hw_timer_t *fatigueTimer = NULL;  //declared timer

TFT_eSPI screen = TFT_eSPI();  //TFT_eSPI type object names Screen used to control the TFT Display.
TFT_eSprite sprite = TFT_eSprite(&screen);
//To begin I2C communication with ESP32 first you must construct the Wire object of class TwoWire.
//If we choose to add more I2C periferals we would define them as TwoWire(1),TwoWire(2), etc.
TwoWire PCF8574 = TwoWire(0);

// Now if we try to initialize I2C communication our Wire library would overwrite the Adafruit_PCF8574 library,
// so we must construct a wire object of class Adafruit_PCF8574.
Adafruit_PCF8574 pcf;

//static constexpr int buttonA = 6;
//SdFat SD;                           // SD card filesystem
//Adafruit_ImageReader imReader(SD);  // Image-reader object, pass in SD filesys

bool inter = 0;  //keeps track of the state of the Interrupt flag.
bool butA = 0;   //keeps track of the state of the button press for buttonA.
bool butB = 0;   //keeps track of the state of the button press for buttonB.
bool butC = 0;   //keeps track of the state of the button press for buttonC.

volatile uint32_t stomach = 100;  //Variable that keeps track of the hunger meter.
volatile uint32_t fatigue = 7;    //Variable that keeps track of the fatigue meter.

bool screenLoaded = false;  //Variable that keeps track wether a screen has been loaded or not.
int currentScreen = 0;      //Variable that keeps track of what is the current screen.
int steps = 0;              //Variable that keeps track of steps.
int progress;
int c = 0;

const unsigned long nextFrm = 500;
unsigned long frm = 0;



void setup() {
  Serial.begin(115200);          //Begins serial communication.
  Wire.begin(I2C_SDA, I2C_SCL);  //Initializes Data & Clock pins for I2C communication.


  Serial.println("Adafruit PCF8574 test");

  //Initializes Wire communication. Takes in the parameters of DATA pin, CLOCK Pin, Clock Speed.
  if (!PCF8574.begin(I2C_SDA, I2C_SCL, 100000)) {
    Serial.println("Couldn't find it.");
  }
  //Initializes PCF8574 communication. Takes in the parameter of Address. (0x20 being the default).
  if (!pcf.begin(0x20)) {
    Serial.println("Couldn't find PCF8574");
  }
  Serial.println("I Guess We Found It!");

  //Sets the pin mode of a pin either INPUT, INPUT with built in Pullup resistor or OUTPUT.
  //Takes the parameters of selected pin, selected mode.
  pcf.pinMode(PCF_BUTTON_A, INPUT);
  pcf.pinMode(PCF_BUTTON_B, INPUT);
  pcf.pinMode(PCF_BUTTON_C, INPUT);
  pcf.pinMode(STEP_PIN, INPUT);
  pinMode(ADA_INTER_PIN, INPUT);  //sets the interupt pin to input and sets on the internal pullup resistor.
  attachInterrupt(digitalPinToInterrupt(ADA_INTER_PIN), inputRead, FALLING);

  if (!SD.begin(SD_CS)) {
    Serial.println(F("SD.begin failed!"));
    while (1) delay(0);
  }
  Serial.println("\r\nInitialisation done.");

  // Start the tft display
  screen.init();
  // Set the TFT display rotation in landscape mode
  screen.setRotation(3);
  // Clear the screen before writing to it
  screen.fillScreen(TFT_BLACK);
  screen.setTextColor(TFT_ORANGE, TFT_BLACK);
  screen.setTextSize(4);
  screen.setSwapBytes(true);
  // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setJpgScale(1);
  // The decoder must be given the exact name of the rendering function above
  TJpgDec.setCallback(tft_output);
  // Set X and Y coordinates for center of display
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;

  stomachTimer = timerBegin(1000000);                     //start the timer
  timerAttachInterrupt(stomachTimer, &stomachCountDown);  //attach interrupt tp timer and function
  timerAlarm(stomachTimer, 180000000, true, 0);           //alarms are flags at specific counts in the timer.

  fatigueTimer = timerBegin(1000000);
  timerAttachInterrupt(fatigueTimer, &fatigueCountDown);
  timerAlarm(fatigueTimer, 420000000, true, 0);

  screen.fillScreen(TFT_WHITE);
  TJpgDec.drawSdJpg(-1, -25, "/foBackgr.jpg");
  sprite.createSprite(43, 64);
  sprite.setSwapBytes(true);
  // sprite.pushImage(0, 0, 114, 156, knghtid);
  // sprite.pushSprite(100, 90, TFT_WHITE);
  //screen.drawBitmap(0,0,/frBackgr,320,240,TFT_WHITE);

  // imReader.drawBMP("/foBackgr.bmp", *tft, 0, 0);
}

bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t *bitmap) {
  // Stop further decoding as image is running off bottom of screen
  if (y >= screen.height()) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  screen.pushImage(x, y, w, h, bitmap);

  // This might work instead if you adapt the sketch to use the Adafruit_GFX library
  // tft.drawRGBBitmap(x, y, bitmap, w, h);

  // Return 1 to decode next block
  return 1;
}



void inputRead(void) {
  inter = 1;
}


static void buttonA_Handler(uint8_t btnId, uint8_t btnState) {

  if (btnState == BTN_PRESSED) {
    //Serial.println("Pushed button A");
    butA = 1;
  } else {
    // btnState == BTN_OPEN.
    //Serial.println("Released button A");
    butA = 0;
  }
}



static void buttonB_Handler(uint8_t btnId, uint8_t btnState) {

  if (btnState == BTN_PRESSED) {
    //Serial.println("Pushed button B");
    butB = 1;
  } else {
    // btnState == BTN_OPEN.
    //Serial.println("Released button B");
    butB = 0;
  }
}


static void buttonC_Handler(uint8_t btnId, uint8_t btnState) {

  if (btnState == BTN_PRESSED) {
    //Serial.println("Pushed button C");
    butC = 1;
  } else {
    // btnState == BTN_OPEN.
    //Serial.println("Released button C");
    butC = 0;
  }
}

static void step_Handler(uint8_t btnId, uint8_t btnState) {

  if (btnState == BTN_PRESSED) {
    Serial.println("STEP");
    steps++;
  } else {
    //btnState == BTN_OPEN.
    //Serial.println("Released button C");
  }
}

static Button myButtonA(0, buttonA_Handler);
static Button myButtonB(1, buttonB_Handler);
static Button myButtonC(2, buttonC_Handler);
static Button myStep(3, step_Handler);



void manageInter() {
  int buttonA = myButtonA.update(pcf.digitalRead(PCF_BUTTON_A));
  int buttonB = myButtonB.update(pcf.digitalRead(PCF_BUTTON_B));
  int buttonC = myButtonC.update(pcf.digitalRead(PCF_BUTTON_C));
  int step_Button = myStep.update(pcf.digitalRead(STEP_PIN));

  // Serial.println(buttonA);
  if (buttonA == 0) {
    inter = 0;
  } else if (buttonB == 0) {
    inter = 0;
  } else if (buttonC == 0) {
    inter = 0;
  } else if (step_Button == 0) {
    inter = 0;
  }
  //myButtonA.update(pcf.digitalRead(PCF_BUTTON_B));
  //myButtonA.update(pcf.digitalRead(PCF_BUTTON_C));
}


void loadScreen(bool &screenLoaded, int &currentScreen, int &steps, volatile uint32_t &stomach) {
  if (screenLoaded == false && currentScreen == 0) {  // MAIN screen
    screen.fillScreen(TFT_BLACK);
    screen.setTextSize(4);
    screen.drawString("MAIN.", 80, 100);
    Serial.println("MAIN.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 1) {  //STEP screen
    screen.fillScreen(TFT_BLACK);
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
    screen.drawString("MAIN.", 80, 100);  // colors over the text in black.
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("STEP.", 80, 100);
    screen.drawNumber(steps, 80, 150);
    Serial.println("STEP.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 2) {  //Progress screen
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
    screen.drawString("STEP.", 80, 100);
    screen.drawNumber(steps, 80, 150);
    //screen.fillScreen(TFT_BLACK);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("PROGRESS.", 90, 30);
    screen.drawNumber(steps, 80, 80);
    screen.drawString("OF", 110, 120);
    //getProgress(steps, progress);
    //screen.drawNumber(progress, 80, 165);
    screen.drawNumber(getProgress(steps, progress), 80, 165);
    Serial.println("STEP PROGRESS.");
    Serial.println(progress);
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 3) {  //STAT screen
    screen.fillScreen(TFT_BLACK);
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
    screen.drawString("STEP.", 80, 100);
    screen.drawNumber(steps, 80, 150);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("STAT.", 80, 100);
    Serial.println("STAT.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 4) {  // VIEW STATS screen
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
    screen.drawString("STAT.", 80, 100);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("HUNGER.", 80, 40);
    screen.drawString("FATIGUE.", 80, 130);
    //screen.drawNumber(stomach, 100, 80);
    screen.drawNumber(stomach, 100, 80);
    screen.drawNumber(fatigue, 100, 170);
    Serial.println(" VIEW STATS.");
    Serial.println(stomach);
    //screen.drawNumber(stomach, 80, 150);
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 5) {  // CAMP screen
    // screen.setTextColor(TFT_BLACK, TFT_BLACK);
    // screen.drawString("HUNGER.", 80, 100);
    screen.fillScreen(TFT_BLACK);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("CAMP.", 80, 100);
    Serial.println("CAMP.");
    //screen.drawNumber(fatigue, 80, 150);
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 6) {  //EAT screen
    // screen.setTextColor(TFT_BLACK, TFT_BLACK);
    // screen.drawString("SLEEP.", 80, 100);
    screen.fillScreen(TFT_BLACK);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("EAT.", 80, 100);
    Serial.println("EAT.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 7) {  //CONFIG screen
    screen.fillScreen(TFT_BLACK);
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
    screen.drawString("CAMP.", 80, 100);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("CONFIG.", 80, 100);
    //screen.drawNumber(steps, 80, 150);
    Serial.println("CONFIG.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 8) {  //SETTINGS screen
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
    screen.drawString("CONFIG.", 80, 100);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("SETTINGS.", 80, 100);
    //screen.drawNumber(steps, 80, 150);
    Serial.println("SETTINGS.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 9) {
    screen.fillScreen(TFT_BLACK);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("SLEEP.", 80, 100);
    Serial.println("SLEEP.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 10) {
    screen.fillScreen(TFT_BLACK);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("SLEEP.", 80, 100);
    Serial.println("SLEEP.");
    screenLoaded = true;
  }
}

void manageScreen(bool &screenLoaded, int &currentScreen, bool &butA, bool &butB, bool &butC, volatile uint32_t &stomach, volatile uint32_t &fatigue) {
  if (screenLoaded == true && currentScreen == 0) {  //MAIN screen
    if (butA == 1) {                                 //Loads STEP screen.
      screenLoaded = false;
      currentScreen = 1;
      butA = 0;
    }
  } else if (screenLoaded == true && currentScreen == 1) {  //STEP screen
    if (butA == 1) {                                        //Loads STATS screen
      screenLoaded = false;
      currentScreen = 3;
      butA = 0;
    } else if (butB == 1) {  //Loads PROGRESS screen.
      screenLoaded = false;
      currentScreen = 2;
      butB = 0;
    } else if (butC == 1) {  //goes back to MAIN.
      screenLoaded = false;
      currentScreen = 0;
      butC = 0;
    }
  } else if (screenLoaded == true && currentScreen == 2) {  //PROGRESS screen
    if (butB == 1) {                                        //loads STEP screen.
      screenLoaded = false;
      currentScreen = 1;
      butB = 0;
      //Serial.println("currentScreen");
      //Serial.println(currentScreen);
    } else if (butC == 1) {  // goes back to MAIN.
      screenLoaded = false;
      currentScreen = 0;
      butC = 0;
    }
  } else if (screenLoaded == true && currentScreen == 3) {  //STAT screen
    if (butA == 1) {                                        //loads CAMP screen.
      screenLoaded = false;
      currentScreen = 5;
      butA = 0;
    } else if (butB == 1) {  //loads  VIEW STATS screen.
      screenLoaded = false;
      currentScreen = 4;
      butB = 0;
    } else if (butC == 1) {  // Goes back to MAIN.
      screenLoaded = false;
      currentScreen = 0;
      butC = 0;
      //Serial.println("currentScreen");
      //Serial.println(currentScreen);
    }
  } else if (screenLoaded == true && currentScreen == 4) {  //VIEW STATS screen
    if (butB == 1) {                                        // loads STAT screen
      screenLoaded = false;
      currentScreen = 3;
      butB = 0;
    } else if (butC == 1) {  // goes back to MAIN.
      screenLoaded = false;
      currentScreen = 0;
      butC = 0;
    }
  } else if (screenLoaded == true && currentScreen == 5) {  //CAMP screen loaded.
    if (butA == 1) {                                        //CONFIG screen loaded.
      screenLoaded = false;
      currentScreen = 7;
      butA = 0;
    } else if (butB == 1) {  //loads ESTC screen
      screenLoaded = false;
      currentScreen = 6;
      butB = 0;
    } else if (butC == 1) {  // goes back to MAIN
      screenLoaded = false;
      currentScreen = 0;
      butC = 0;
    }
  } else if (screenLoaded == true && currentScreen == 6) {  //EAT screen
    if (butA == 1) {                                        // loads SLEEP screen.
      screenLoaded = false;
      currentScreen = 9;
      butA = 0;
    } else if (butB == 1) {  //executes EAT function
      eat(stomach);
      butB = 0;
    } else if (butC == 1) {  //goes back to MAIN
      screenLoaded = false;
      currentScreen = 0;
      butC = 0;
    }
  } else if (screenLoaded == true && currentScreen == 7) {  //CONFIG screen
    if (butA == 1) {                                        // goes back to MAIN
      screenLoaded = false;
      currentScreen = 0;
      butA = 0;
    } else if (butB == 1) {  // loads SETTINGS screen
      screenLoaded = false;
      currentScreen = 8;
      butB = 0;
    } else if (butC == 1) {  // goes back to MAIN
      screenLoaded = false;
      currentScreen = 0;
      butC = 0;
    }
  } else if (screenLoaded == true && currentScreen == 8) {  // SETTING screen
    if (butB == 1) {                                        // loads CONFIG screen
      screenLoaded = false;
      currentScreen = 7;
      butB = 0;
    } else if (butC == 1) {  // goes back to MAIN
      screenLoaded = false;
      currentScreen = 0;
      butC = 0;
    }
  } else if (screenLoaded == true && currentScreen == 9) {  //SLEEP screen
    if (butA == 1) {                                        // goes back to MAIN
      screenLoaded = false;
      currentScreen = 5;
      butA = 0;
    } else if (butB == 1) {  //Selects SLEEP
      sleep(fatigue);
      butB = 0;
    } else if (butC == 1) {  //goes back to MAIN
      screenLoaded = false;
      currentScreen = 0;
      butC = 0;
    }
  }
}

int eat(volatile uint32_t &stomach) {
  if (stomach <= 0) {
    timerStart(stomachTimer);
  }
  stomach += 10;

  if (stomach >= 100) {
    stomach = 100;
  }

  return stomach;
}

int sleep(volatile uint32_t &fatigue) {
  if (fatigue <= 0) {
    timerStart(fatigueTimer);
  }

  fatigue += 1;

  if (fatigue >= 7) {
    fatigue = 7;
  }

  return fatigue;
}


int getProgress(int &steps, int &progress) {

  while (steps >= 1 && steps <= 20) {
    progress = 20;
    return progress;
  }
  while (steps >= 21 && steps <= 50) {
    progress = 50;
    return progress;
  }
  while (steps >= 51 && steps <= 80) {
    progress = 80;
    return progress;
  }
}

void ARDUINO_ISR_ATTR stomachCountDown() {

  if (stomach <= 0) {
    timerStop(stomachTimer);
  } else {
    //s = true;
    stomach -= 4;
    //stomach--;
  }
}

void ARDUINO_ISR_ATTR fatigueCountDown() {

  if (fatigue <= 0) {
    //stomach = 0;
    timerStop(fatigueTimer);
  } else {
    fatigue--;
  }
}

void knightIdle(int c) {
  for (int i = 0; i < 2000; i++) {
    if (c >= 0 && c <= 499) {
      sprite.pushImage(0, 0, 43, 64, knigthIdle[0]);
      sprite.pushSprite(150, 164, TFT_WHITE);
      c++;
      Serial.println(c);
    } else if (c >= 500 && c <= 999) {
      sprite.pushImage(0, 0, 43, 64, knigthIdle[1]);
      sprite.pushSprite(150, 164, TFT_WHITE);
      c++;
      Serial.println(c);
    } else if (c >= 1000 && c <= 1499) {
      sprite.pushImage(0, 0, 43, 64, knigthIdle[2]);
      sprite.pushSprite(150, 164, TFT_WHITE);
      c++;
      Serial.println(c);
    } else if (c >= 1500 && c <= 2000) {
      sprite.pushImage(0, 0, 43, 64, knigthIdle[3]);
      sprite.pushSprite(150, 164, TFT_WHITE);
      c++;
      Serial.println(c);
    } else if (c >= 2000) {
      i = 0;
      c = 0;
    }
  }
}

void loop() {
  knightIdle(c);

  // for (int i = 0; i < frames; i++) {
  //   sprite.pushImage(0, 0, 43, 64, knigthIdle[i]);
  //   sprite.pushSprite(150, 164, TFT_WHITE);
  // }
  //loadScreen(screenLoaded, currentScreen, steps, stomach);
  //manageScreen(screenLoaded, currentScreen, butA, butB, butC, stomach, fatigue);
  //manageInter();
  //Serial.println(fatigue);
}

// A button Cycles through options
// B button selects options
// C button goes back to the previous screen