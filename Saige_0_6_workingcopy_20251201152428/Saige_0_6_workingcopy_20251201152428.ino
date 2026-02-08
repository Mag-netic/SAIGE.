//Project Saige

//Libraries used
#include <Wire.h>              //Wire library for communicating through I2C
#include <Adafruit_PCF8574.h>  //Library for PCF8574 GPIO expander, opens up 8 GPIO pins through one address 0x20
#include <TFT_eSPI.h>
#include <debounce.h>
// The available pins on the CYD are
// GPIO 35 — on the P3 connector, GPIO 22 — on the P3 and CN1 connector, GPIO 27 — on the CN1 connector.
// GPIO 27 will be used for SERIAL DATA for I2C communication.
// GPIO 22 will be used for SERIAL CLOCK for I2C Communication.
#define I2C_SDA 27  //Sets I2C Data to GPIO pin 27.
#define I2C_SCL 22  //Sets I2c Clock to GPIO pin 22.

#define PCF_BUTTON_A 6    //Initializes GPIO pin 6 of the PCF8574.
#define PCF_BUTTON_B 5    //Initializes GPIO pin 5 of the PCF8574.
#define PCF_BUTTON_C 4    //Initializes GPIO pin 4 of the PCF8574.
#define ADA_INTER_PIN 35  //Sets the PCF8574 interupt pin (INT) to GPIO pin 35.
#define STEP_PIN 0

#define SCREEN_WIDTH 320   //Sets the screen width at 320 pixels.
#define SCREEN_HEIGHT 240  //Sets the screen hight at 240 pixels.
#define FONT_SIZE 2        //Sets the font size.

TFT_eSPI screen = TFT_eSPI();  //TFT_eSPI type object names Screen used to control the TFT Display.

//To begin I2C communication with ESP32 first you must construct the Wire object of class TwoWire.
//If we choose to add more I2C periferals we would define them as TwoWire(1),TwoWire(2), etc.
TwoWire PCF8574 = TwoWire(0);

// Now if we try to initialize I2C communication our Wire library would overwrite the Adafruit_PCF8574 library,
// so we must construct a wire object of class Adafruit_PCF8574.
Adafruit_PCF8574 pcf;

//static constexpr int buttonA = 6;


bool inter = 0;  //keeps track of the state of the Interrupt flag.
bool butA = 0;   //keeps track of the state of the button press for buttonA.
bool butB = 0;   //keeps track of the state of the button press for buttonB.
bool butC = 0;   //keeps track of the state of the button press for buttonC.

unsigned long lastTimeButtonA_StateChanged;             //debounce variable.
unsigned long lastTimeButtonB_StateChanged = millis();  //debounce variable.
unsigned long lastTimeButtonC_StateChanged = millis();  //debounce variable.
unsigned long lastTimeStep_StateChanged = millis();     //debounce variable.
unsigned long debounceDuration = 160;                   //amount of millis() system waits for button press.
unsigned long stepDebounceDuration = 500;               //amount of millis() system waits for step counter.
unsigned long stomachTime = millis();                   //debounce variable.
unsigned long sleepTime = millis();                     //debounce variable.
unsigned long stomachPoint = 180000;                    //amount of time till the stomach counter decreases. (3 Minutes);
unsigned long sleepPoint = 180000;                      //amount of time till the sleep counter decreases. (3 Minutes);
unsigned long currentStomachPoint;


bool screenLoaded = false;  //Variable that keeps track wether a screen has been loaded or not.
int currentScreen = 0;      //Variable that keeps track of what is the current screen.
int steps = 0;              //Variable that keeps track of steps.
int stomach = 100;          //Variable that keeps track of the hunger meter.
int fatigue = 100;
int lastButtonA_Value = 1;


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

  // Start the tft display
  screen.init();
  // Set the TFT display rotation in landscape mode
  screen.setRotation(3);
  // Clear the screen before writing to it
  screen.fillScreen(TFT_BLACK);
  screen.setTextColor(TFT_ORANGE, TFT_BLACK);
  screen.setTextSize(4);

  // Set X and Y coordinates for center of display
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;
}

int getButtonAgain() {
  int buttA_Value = pcf.digitalRead(PCF_BUTTON_A);
  Serial.println(buttA_Value);
}


int getButtonA() {
  int lastButtonA_State = 1;
  int buttonA_Value = pcf.digitalRead(PCF_BUTTON_A);
  int buttonA_State;

  if (buttonA_Value != lastButtonA_State) {
    lastTimeButtonA_StateChanged = millis();
    if (millis() - lastTimeButtonA_StateChanged >= debounceDuration) {

      if (buttonA_Value != buttonA_State) {

        buttonA_State = buttonA_Value;

        if (buttonA_Value == 0) {
          Serial.println("here Button A");
          return 1;
          lastButtonA_State = buttonA_Value;
          inter = 0;
        }
      } else if (buttonA_Value == lastButtonA_State) {
        Serial.println("return");
        return 0;
      }
    }
  }
}

void getButtonB() {
  int lastButtonB_State = 1;
  int buttonB_Value = pcf.digitalRead(PCF_BUTTON_B);
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;

  if (millis() - lastTimeButtonB_StateChanged >= debounceDuration) {
    int buttonB_State = pcf.digitalRead(PCF_BUTTON_B);
    if (inter == 1 && buttonB_Value != lastButtonB_State) {
      lastTimeButtonB_StateChanged = millis();
      lastButtonB_State = buttonB_State;
      if (buttonB_Value == 0) {
        Serial.println("Buttton B");
        screen.drawCentreString("Button B", centerX, 30, FONT_SIZE);
        inter = 0;
      }
    }
  }
}

void getButtonC() {
  int lastButtonC_State = 1;
  int buttonC_Value = pcf.digitalRead(PCF_BUTTON_C);
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;

  if (millis() - lastTimeButtonC_StateChanged >= debounceDuration) {
    int buttonC_State = pcf.digitalRead(PCF_BUTTON_C);
    if (inter == 1 && buttonC_Value != lastButtonC_State) {
      lastTimeButtonC_StateChanged = millis();
      lastButtonC_State = buttonC_State;
      if (buttonC_Value == 0) {
        Serial.println("Buttton C");
        screen.drawCentreString("Button C", centerX, 30, FONT_SIZE);
        inter = 0;
      }
    }
  }
}

void getStep() {

  int lastStep_State = 1;
  int step_Value = pcf.digitalRead(STEP_PIN);
  int centerX = SCREEN_WIDTH / 2;
  int centerY = SCREEN_HEIGHT / 2;

  if (millis() - lastTimeStep_StateChanged >= stepDebounceDuration) {
    int stepState = pcf.digitalRead(STEP_PIN);
    if (inter == 1 && step_Value != lastStep_State) {
      lastTimeStep_StateChanged = millis();
      lastStep_State = stepState;
      if (step_Value == 0) {
        steps++;
        Serial.println(steps);
        //screen.drawCentreString("STEPS" + stepCount, centerX, 25, FONT_SIZE);
        inter = 0;
      }
    }
  }
}

void inputRead(void) {
  inter = 1;
}


static void buttonA_Handler(uint8_t btnId, uint8_t btnState) {

  if (btnState == BTN_PRESSED) {
    Serial.println("Pushed button A");
    butA = 1;
  } else {
    // btnState == BTN_OPEN.
    //Serial.println("Released button A");
    butA = 0;
  }
}


static void buttonB_Handler(uint8_t btnId, uint8_t btnState) {

  if (btnState == BTN_PRESSED) {
    Serial.println("Pushed button B");
    butB = 1;
  } else {
    // btnState == BTN_OPEN.
    //Serial.println("Released button B");
    butB = 0;
  }
}


static void buttonC_Handler(uint8_t btnId, uint8_t btnState) {

  if (btnState == BTN_PRESSED) {
    Serial.println("Pushed button C");
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

// static void pollButtons() {
//   // update() will call buttonHandler() if PIN transitions to a new state and stays there
//   // for multiple reads over 25+ ms.
//   myButtonA.update(pcf.digitalRead(PCF_BUTTON_A));
// }

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


void loadScreen(bool &screenLoaded, int &currentScreen, int &steps, int &stomach) {
  if (screenLoaded == false && currentScreen == 0) {
    screen.fillScreen(TFT_BLACK);
    screen.setTextSize(4);
    screen.drawString("MAIN.", 90, 30);
    Serial.println("main.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 1) {
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
    screen.drawString("MAIN.", 90, 30);  // colors over the text in black.
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("STEP.", 90, 30);
    screen.drawNumber(steps, 60, 80);
    Serial.println("STEP.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 2) {
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
    screen.drawString("STEP.", 90, 30);
    screen.drawNumber(steps, 60, 80);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("PROG.", 90, 30);
    screen.drawNumber(steps, 80, 150);
    Serial.println("STEP PROGRESS.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 3) {
    //screen.setTextColor(TFT_BLACK, TFT_BLACK);
    //screen.drawString("STEP.", 80, 100);
    //screen.drawNumber(steps, 80, 150);
    screen.fillScreen(TFT_BLACK);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("STAT.", 80, 100);
    Serial.println("STAT.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 4) {
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
    screen.drawString("STAT.", 80, 100);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("HUNGER.", 80, 100);
    Serial.println("HUNGER.");
    screen.drawNumber(stomach, 80, 150);
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 5) {
    // screen.setTextColor(TFT_BLACK, TFT_BLACK);
    // screen.drawString("HUNGER.", 80, 100);
    screen.fillScreen(TFT_BLACK);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("SLEEP.", 80, 100);
    Serial.println("SLEEP.");
    screen.drawNumber(fatigue, 80, 150);
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 6) {
    // screen.setTextColor(TFT_BLACK, TFT_BLACK);
    // screen.drawString("SLEEP.", 80, 100);
    screen.fillScreen(TFT_BLACK);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("CONFIG.", 80, 100);
    Serial.println("CONFIG.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 7) {
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
    screen.drawString("MAIN.", 80, 100);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("STEP.", 80, 100);
    screen.drawNumber(steps, 80, 150);
    Serial.println("STEP.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 8) {
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
    screen.drawString("MAIN.", 80, 100);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("STEP.", 80, 100);
    screen.drawNumber(steps, 80, 150);
    Serial.println("STEP.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 9) {
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
    screen.drawString("MAIN.", 80, 100);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("STEP.", 80, 100);
    screen.drawNumber(steps, 80, 150);
    Serial.println("STEP.");
    screenLoaded = true;
  } else if (screenLoaded == false && currentScreen == 10) {
    screen.setTextColor(TFT_BLACK, TFT_BLACK);
    screen.drawString("MAIN.", 80, 100);
    screen.setTextColor(TFT_ORANGE, TFT_BLACK);
    screen.drawString("STEP.", 80, 100);
    screen.drawNumber(steps, 80, 150);
    Serial.println("STEP.");
    screenLoaded = true;
  }
}

void manageScreen(bool &screenLoaded, int &currentScreen, bool &butA, bool &butC) {
  if (screenLoaded == true && currentScreen == 0) {
    if (butA == 1) {
      screenLoaded = false;
      currentScreen = 1;
      butA = 0;
    }
  } else if (screenLoaded == true && currentScreen == 1) {
    if (butA == 1 && butC == 0) {
      screenLoaded = false;
      currentScreen = 3;
      butA = 0;
    } else if (butC == 1 && butA == 0) {
      screenLoaded = false;
      currentScreen = 2;
      butC = 0;
    }
  } else if (screenLoaded == true && currentScreen == 2) {
    if (butA == 1) {
      screenLoaded = false;
      currentScreen = 3;
      butA = 0;
    }
  } else if (screenLoaded == true && currentScreen == 3) {
    if (butA == 1) {
      screenLoaded = false;
      currentScreen = 4;
      butA = 0;
    }
  } else if (screenLoaded == true && currentScreen == 4) {
    if (butA == 1) {
      screenLoaded = false;
      currentScreen = 5;
      butA = 0;
    }
  } else if (screenLoaded == true && currentScreen == 5) {
    if (butA == 1) {
      screenLoaded = false;
      currentScreen = 0;
      butA = 0;
    }
  }
}

void getHunger() {
  if (millis() - stomachTime >= stomachPoint) {
    stomachTime = millis();
    stomach--;
    Serial.println(stomach);
  }
}

void getSleep() {
  if (millis() - sleepTime >= sleepPoint) {
    sleepTime = millis();
    fatigue--;
    Serial.println(fatigue);
  }
}

void loop() {
  loadScreen(screenLoaded, currentScreen, steps, stomach);
  manageScreen(screenLoaded, currentScreen, butA, butC);
  manageInter();

  //pollButtons();
  //getButtonAgain();
  //Serial.println(inter);
  //Serial.println(getButtonA());
  //delay(100);
}
