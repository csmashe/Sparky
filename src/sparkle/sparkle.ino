#include <Wire.h>
#include "SSD1306Wire.h" // https://github.com/ThingPulse/esp8266-oled-ssd1306
//#include "SH1106Wire.h"
#include "BluetoothSerial.h" // https://github.com/espressif/arduino-esp32
#include "font.h"
#include "presets.h"
#include <BfButton.h> //https://github.com/mickey9801/ButtonFever

// Device Info Definitions
const String DEVICE_NAME = "Sparkel";
const String VERSION = "0.3.1";
const String SPARK_BT_NAME = "Spark 40 Audio";

// Check ESP32 Bluetooth configuration
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// OLED Screen Definitions (SH1106 driven screen can be used in place of a SSD1306 screen, if desired)
SSD1306Wire oled(0x3c, SDA, SCL); // ADDRESS, SDA, SCL 
//SH1106Wire oled(0x3c, SDA, SCL); // ADDRESS, SDA, SCL 

// Button GPIO and Object Definitions
#define NUM_OF_BUTTONS 4

#define BUTTON_1_GPIO 19
#define BUTTON_2_GPIO 18
#define BUTTON_3_GPIO 5
#define BUTTON_4_GPIO 4
const int BUTTON_GPI0_LIST[] = {BUTTON_1_GPIO, BUTTON_2_GPIO, BUTTON_3_GPIO, BUTTON_4_GPIO}; 

BfButton btn_1(BfButton::STANDALONE_DIGITAL, BUTTON_1_GPIO, false, HIGH);
BfButton btn_2(BfButton::STANDALONE_DIGITAL, BUTTON_2_GPIO, false, HIGH);
BfButton btn_3(BfButton::STANDALONE_DIGITAL, BUTTON_3_GPIO, false, HIGH);
BfButton btn_4(BfButton::STANDALONE_DIGITAL, BUTTON_4_GPIO, false, HIGH);
BfButton BTN_LIST[] = {btn_1, btn_2, btn_3, btn_4};

// ESP32 Bluetooth Serial Object
BluetoothSerial SerialBT;

//New Definitions
#define FX_ON 67
#define FX_OFF 66


// Device State Variables
int selected_tone_preset;  //remove?
bool connected;
bool debug = true;
//new Variables
char* PresetName; char* PresetName1; char* PresetName2;
char* CurrentPresetName;  char* CurrentPresetName1; char* CurrentPresetName2;
char* FXDriveType;
char* FXModType;
char* FXDelayType;
char* FXReverbType;
byte FXDriveStatus[]={FX_OFF};
byte FXModStatus[]={FX_OFF};
byte FXDelayStatus[]={FX_OFF};
byte FXReverbStatus[]={FX_OFF};
bool SelectMode = false;
int incomingBLEByte = 0;


//Functions On Press Of One Of The 4 Buttons
void switchingPressHandler (BfButton *btn, BfButton::press_pattern_t pattern) {
     
   
   //get the button that was pressed
   int buttonId=0; 
   int pressed_btn_gpio = btn->getID();
   for(int i = 0; i< NUM_OF_BUTTONS; i++) {
      if (pressed_btn_gpio == BUTTON_GPI0_LIST[i]) {
          buttonId = i;
      }
    }

    //printDebug(String(pressed_btn_gpio));

    if(!SelectMode){
      //if long press go into select mode
      printDebug(String(pattern));
      if(pattern == BfButton::LONG_PRESS && buttonId == 0){
        //printDebug("Long Press Select Mode");
        //store the current preset info for later
        CurrentPresetName = PresetName;
        CurrentPresetName1 = PresetName1;
        CurrentPresetName2 = PresetName2;
        
        SelectMode = true;
       
        printPresetSelectScreen();
        return;
      }

      //toggle the fx
      if(buttonId == 0){
        //toggle drive
        printDebug("toggle drive");
      }
      if(buttonId == 1){
        //toggle mod
        printDebug("toggle mod");
      }
      if(buttonId == 2){
        //toggle delay
        printDebug("toggle delay");
      }
      if(buttonId == 3){
        //toggle reverb
        printDebug("toggle reverb");
      }
    }else{
      if(pattern == BfButton::LONG_PRESS && buttonId == 0){
        SelectMode = false;
        //printDebug("save preset");
        SendPresetToAmp(PresetName);
        printPresetToOLED();
      }

      if(buttonId == 1){
        // printDebug("previous");
         pressPrevious();
       
      }
      if(buttonId == 2){
         //printDebug("next");
          pressNext();
      }
  
      if(pattern == BfButton::LONG_PRESS && buttonId == 3){
        SelectMode = false;
        //set back to what we where using and exit
        PresetName = CurrentPresetName;
        PresetName1 = CurrentPresetName1;
        PresetName2 = CurrentPresetName2;
        printPresetToOLED();  
      }
      
      
    }

 
}

//Press The Previous Button in Select Mode
void pressPrevious(){

  for (int i = presetArraySize; i --> 0; )
  {
      if(PresetName == presetArray[i]){
        int arrayPosition = i;
         if((i-1) == 0){
           arrayPosition = presetArraySize;
         }
    
         PresetName = presetArray[arrayPosition];
         PresetName1 = presetDisplay1Array[arrayPosition];
         PresetName2 = presetDisplay2Array[arrayPosition];
         printPresetSelectScreen();
         return;
      }
    }
    
}

//Press The Next Button in Select Mode
void pressNext(){

    for(int i = 0; i < presetArraySize; i++) {
      if(PresetName == presetArray[i]){
        int arrayPosition = i;
         if((i+1) == presetArraySize){
           arrayPosition = 0;
         }
    
         PresetName = presetArray[arrayPosition];
         PresetName1 = presetDisplay1Array[arrayPosition];
         PresetName2 = presetDisplay2Array[arrayPosition];
         printPresetSelectScreen();
         return;
      }
    }
}

//Show The Preset Selection Screen
void printPresetSelectScreen(){

   oled.clear();
   oled.setFont(ArialMT_Plain_16);
   oled.setTextAlignment(TEXT_ALIGN_CENTER);
   oled.drawString(64, 0, "Current Preset");
   oled.drawString(64, 25, PresetName1);
   oled.drawString(64, 45, PresetName2);
    oled.display();
}

//Show The Selected Preset Screen And The Toggles For The Pedals
void printPresetToOLED() {

  oled.clear();

  oled.setFont(ArialMT_Plain_16);
  if (FXDriveStatus[0]==FX_ON) oled.drawString(0, 0, "Dr");
  if (FXModStatus[0]==FX_ON) oled.drawString(25, 0, "Mod");
  if (FXDelayStatus[0]==FX_ON) oled.drawString(65, 0, "Rev");
  if (FXReverbStatus[0]==FX_ON) oled.drawString(105, 0, "Del");

  //show the preset name
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 25, PresetName1);
  oled.drawString(64, 45, PresetName2);
  
  
  oled.display();
}



//Start The Display
void displayStartup() {
  // Initialize device OLED display, and flip screen, as OLED library starts "upside-down" (for some reason?)
  oled.init();
  oled.flipScreenVertically();

  // Show "TinderBox ESP v<version_num>" message on device screen
  oled.clear();
  oled.setFont(ArialMT_Plain_24);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 12, DEVICE_NAME);
  oled.setFont(ArialMT_Plain_16);
  oled.setTextAlignment(TEXT_ALIGN_CENTER);
  oled.drawString(64, 36, "ESP32 v" + VERSION);
  oled.display();
  
  delay(4000);
}

//Set Up Buttons
void inputSetup() {
  // Setup callback for single press detection on all four input buttons
  for(int i = 0; i < NUM_OF_BUTTONS; i++) {
    BTN_LIST[i].onPress(switchingPressHandler)
    .onPressFor(switchingPressHandler, 1000); ;
  }
}


//Button Event Call Back
void btEventCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param){
  // On BT connection close
  if(event == ESP_SPP_CLOSE_EVT ){
    // TODO: Until the cause of connection instability (compared to Pi version) over long durations 
    // is resolved, this should keep your pedal and amp connected fairly well by forcing reconnection
    // in the main loop
    connected = false;
    selected_tone_preset = 0;
  }
}

//Initialize Bluetooth
void btInit() {
  // Register BT event callback method
  SerialBT.register_callback(btEventCallback);
  if(!SerialBT.begin(DEVICE_NAME, true)){ // Detect for BT failure on ESP32 chip
    // Show "BT Init Failed!" message on device screen
    oled.clear();
    oled.setFont(ArialMT_Plain_24);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(64, 6, "BT Init");
    oled.drawString(64, 30, "Failed!");
    oled.display();
    
    // Loop infinitely until device shutdown/restart
    while(true){};
  }
}

//Conect To Spark Amp
void connectToAmp() {
  // Loop until device establishes connection with amp
  while(!connected) {
    // Show "Connecting" message on device screen
    oled.clear();
    oled.setFont(ArialMT_Plain_24);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(64, 20, "Connecting");
    oled.display();

    // Attempt BT connection to amp
    connected = SerialBT.connect(SPARK_BT_NAME);

    // If BT connection with amp is successful
    if (connected && SerialBT.hasClient()) {
      // Show "Connected" message on device screen
      oled.clear();
      oled.setFont(ArialMT_Plain_24);
      oled.setTextAlignment(TEXT_ALIGN_CENTER);
      oled.drawString(64, 20, "Connected");
      oled.display();
      
      delay(2000);

      //Set inital Tone
      PresetName="BangBang"; PresetName1="Bang Bang"; PresetName2="";
      SendPresetToAmp(PresetName);
      
      // Display inital Tone Preset Screen
      printPresetToOLED();
    } else { // If amp is not found, or other connection issue occurs
      // Set 'connected' to false to continue amp connection loop
      connected = false;

      // Show "Failed Rescanning" message on device screen
      oled.clear();
      oled.setFont(ArialMT_Plain_24);
      oled.setTextAlignment(TEXT_ALIGN_CENTER);
      oled.drawString(64, 6, "Failed");
      oled.drawString(64, 30, "Rescanning");
      oled.display();
      
      delay(4000);
    }
  }
}



void setup() {
  // Start serial debug console monitoring
  Serial.begin(115200);
  while (!Serial);

  // Set initial device state values
  connected = false;
  selected_tone_preset = 0;

  // Setup Device I/O
  inputSetup();
  displayStartup();
  btInit();
}

void loop() {
  // Check if amp is connected to device
  if(!connected) {
    // If not, attempt to establish a connection
    connectToAmp();
  } else { // If amp is connected to device over BT
    // Scan all input buttons for presses
    for(int i = 0; i < NUM_OF_BUTTONS; i++) {
      BTN_LIST[i].read();
    }
  
    // Read in response data from amp, to clear BT message buffer
    if (SerialBT.available()) {
      SerialBT.read();
    }
  }
}

//Print Debug Values To Screen
void printDebug(String value){

  if(!debug) return;

    oled.clear();
    oled.setFont(ArialMT_Plain_24);
    oled.setTextAlignment(TEXT_ALIGN_CENTER);
    oled.drawString(64, 6, value);
    oled.display();
    
    delay(4000);
}

void ReadSparkResponse() {
  delay(100);while (SerialBT.available()) {incomingBLEByte=SerialBT.read();} // Serial.print(incomingBLEByte,HEX);Serial.print(",");
  //Serial.println("ReadSparkResponse END.");
}

void SendPresetToAmp(String which){
  if (which == "BangBang")
        SetPresetBangBang();
   if (which =="BBKing")
      SetPresetBBKing();
     
}

//Presets
void SetPresetBangBang() {
  FXDriveType="Booster"; FXDriveStatus[0]=FX_ON;
  FXModType="Tremolo"; FXModStatus[0]=FX_ON;
  FXDelayType="DigitalDelay"; FXDelayStatus[0]=FX_OFF;
  FXReverbType="PlateShort"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(BangBangA,sizeof(BangBangA)); ReadSparkResponse();
  SerialBT.write(BangBangB,sizeof(BangBangB)); ReadSparkResponse();
  SerialBT.write(BangBangC,sizeof(BangBangC)); ReadSparkResponse();
  SerialBT.write(BangBangD,sizeof(BangBangD)); ReadSparkResponse();
}

void SetPresetBBKing() {
  FXDriveType="Booster"; FXDriveStatus[0]=FX_ON;
  FXModType="Phaser"; FXModStatus[0]=FX_OFF;
  FXDelayType="EchoTape"; FXDelayStatus[0]=FX_OFF;
  FXReverbType="RoomStudioA"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(BBKingA,sizeof(BBKingA)); ReadSparkResponse();
  SerialBT.write(BBKingB,sizeof(BBKingB)); ReadSparkResponse();
  SerialBT.write(BBKingC,sizeof(BBKingC)); ReadSparkResponse();
  SerialBT.write(BBKingD,sizeof(BBKingD)); ReadSparkResponse();
}
