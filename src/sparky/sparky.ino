#include <Wire.h>
#include "SSD1306Wire.h" // https://github.com/ThingPulse/esp8266-oled-ssd1306
//#include "SH1106Wire.h"
#include "BluetoothSerial.h" // https://github.com/espressif/arduino-esp32
#include "font.h"
#include "presets.h"
#include <BfButton.h> //https://github.com/mickey9801/ButtonFever

// Device Info Definitions
const String DEVICE_NAME = "Sparky";
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
bool connected;
//new Variables
bool debug = false;
bool ClonerSetToLow=true;
bool SelectMode = false;
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
      //printDebug(String(pattern));
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
        DriveStatusToggle();
      }
      if(buttonId == 1){
        ModStatusToggle();
      }
      if(buttonId == 2){
        DelayStatusToggle();
      }
      if(buttonId == 3){
        ReverbStatusToggle();
      }
       printPresetToOLED();  
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
        int arrayPosition = i-1;
         if(arrayPosition == 0){
           arrayPosition = presetArraySize;
         }
         printDebug(String(arrayPosition));
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
        int arrayPosition = i + 1;
         if(arrayPosition == presetArraySize){
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
  if (FXDriveStatus[0]==FX_ON) oled.drawString(10, 0, "Dr");
  if (FXModStatus[0]==FX_ON) oled.drawString(40, 0, "Mod");
  if (FXDelayStatus[0]==FX_ON) oled.drawString(80, 0, "Rev");
  if (FXReverbStatus[0]==FX_ON) oled.drawString(115, 0, "Del");

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

//Read Response From Spark
void ReadSparkResponse() {
  delay(100);while (SerialBT.available()) {incomingBLEByte=SerialBT.read();} // Serial.print(incomingBLEByte,HEX);Serial.print(",");
  //Serial.println("ReadSparkResponse END.");
}

//Send Preset Values To Amp
void SendPresetToAmp(String which){
  if (which == "BangBang")
        SetPresetBangBang();
   if (which =="BBKing")
      SetPresetBBKing();
     
}

//Toggle The Drive Status
void DriveStatusToggle() {
  Serial.print(FXDriveType);
  
  if (FXDriveStatus[0]==FX_ON) {FXDriveStatus[0]=FX_OFF;Serial.println("=OFF");}
  else {FXDriveStatus[0]=FX_ON;Serial.println("=ON");}


  if (FXDriveType=="TubeDrive") {SerialBT.write(TubeDriveCodes,sizeof(TubeDriveCodes));SerialBT.write(FXDriveStatus,1);SerialBT.write(EndOfMessage,1);}
  else if (FXDriveType=="Booster") {SerialBT.write(BoosterCodes,sizeof(BoosterCodes));SerialBT.write(FXDriveStatus,1);SerialBT.write(EndOfMessage,1);}
  else if (FXDriveType=="Fuzz") {SerialBT.write(FuzzCodes,sizeof(FuzzCodes));SerialBT.write(FXDriveStatus,1);SerialBT.write(EndOfMessage,1);}
  else if (FXDriveType=="OverDrive") {SerialBT.write(OverDriveCodes,sizeof(OverDriveCodes));SerialBT.write(FXDriveStatus,1);SerialBT.write(EndOfMessage,1);}

  delay(500);
}

//Toggle The Mod Status
void ModStatusToggle() {
  Serial.print(FXModType);
  
  if (FXModStatus[0]==FX_ON) {FXModStatus[0]=FX_OFF;Serial.println("=OFF");}
  else {FXModStatus[0]=FX_ON;Serial.println("=ON");}


  if (FXModType=="Vibe") {SerialBT.write(VibeSwitchCodes,sizeof(VibeSwitchCodes));SerialBT.write(FXModStatus,1);SerialBT.write(EndOfMessage,1);}
  else if (FXModType=="DigitalChorus") {SerialBT.write(DigitalChorusSwitchCodes,sizeof(DigitalChorusSwitchCodes));SerialBT.write(FXModStatus,1);SerialBT.write(EndOfMessage,1);}
  else if (FXModType=="Tremolo") {SerialBT.write(TremoloSwitchCodes,sizeof(TremoloSwitchCodes));SerialBT.write(FXModStatus,1);SerialBT.write(EndOfMessage,1);}
  else if (FXModType=="Tremolator") {SerialBT.write(TremolatorSwitchCodes,sizeof(TremolatorSwitchCodes));SerialBT.write(FXModStatus,1);SerialBT.write(EndOfMessage,1);}
  else if (FXModType=="Phaser") {SerialBT.write(PhaserSwitchCodes,sizeof(PhaserSwitchCodes));SerialBT.write(FXModStatus,1);SerialBT.write(EndOfMessage,1);}
  else if (FXModType=="Cloner") {SerialBT.write(ClonerSwitchCodes,sizeof(ClonerSwitchCodes));SerialBT.write(FXModStatus,1);SerialBT.write(EndOfMessage,1);}
  else if (FXModType=="Flanger") {SerialBT.write(FlangerSwitchCodes,sizeof(FlangerSwitchCodes));SerialBT.write(FXModStatus,1);SerialBT.write(EndOfMessage,1);}

  delay(500);
}

//Toggle The Delay Status  
void DelayStatusToggle() {
  Serial.print(FXDelayType);

  if (FXDelayStatus[0]==FX_ON) {FXDelayStatus[0]=FX_OFF;Serial.println("=OFF");}
  else {FXDelayStatus[0]=FX_ON;Serial.println("=ON");}

  
  if (FXDelayType=="DelayEcho") {SerialBT.write(DelayEchoCodes,sizeof(DelayEchoCodes));SerialBT.write(FXDelayStatus,1);SerialBT.write(EndOfMessage,1);}
  else if (FXDelayType=="DigitalDelay") {SerialBT.write(DigitalDelayCodes,sizeof(DigitalDelayCodes));SerialBT.write(FXDelayStatus,1);SerialBT.write(EndOfMessage,1);}
  else if (FXDelayType=="EchoTape") {SerialBT.write(EchoTapeCodes,sizeof(EchoTapeCodes));SerialBT.write(FXDelayStatus,1);SerialBT.write(EndOfMessage,1);}
  else if (FXDelayType=="VintageDelay") {SerialBT.write(VintageDelayCodes,sizeof(VintageDelayCodes));SerialBT.write(FXDelayStatus,1);SerialBT.write(EndOfMessage,1);}
  else if (FXDelayType=="MultiHeadDelay") {SerialBT.write(MultiHeadDelayCodes,sizeof(MultiHeadDelayCodes));SerialBT.write(FXDelayStatus,1);SerialBT.write(EndOfMessage,1);}

  delay(500);
}

//Toggle The Reverb Status 
void ReverbStatusToggle() {
  Serial.print(FXReverbType);
  
  if (FXReverbStatus[0]==FX_ON) {FXReverbStatus[0]=FX_OFF;Serial.println("=OFF");}
  else {FXReverbStatus[0]=FX_ON;Serial.println("=ON");}

  // Note - seems that this turns off all reverbs
  SerialBT.write(ReverbCodes,sizeof(ReverbCodes));SerialBT.write(FXReverbStatus,1);SerialBT.write(EndOfMessage,1);

  delay(500);
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

void SetPresetBetterCallSaul() {
  FXDriveType="Booster"; FXDriveStatus[0]=FX_ON;
  FXModType="Phaser"; FXModStatus[0]=FX_OFF;
  FXDelayType="EchoTape"; FXDelayStatus[0]=FX_OFF;
  FXReverbType="Chamber"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(BetterCallSaulA,sizeof(BetterCallSaulA)); ReadSparkResponse();
  SerialBT.write(BetterCallSaulB,sizeof(BetterCallSaulB)); ReadSparkResponse();
  SerialBT.write(BetterCallSaulC,sizeof(BetterCallSaulC)); ReadSparkResponse();
  SerialBT.write(BetterCallSaulD,sizeof(BetterCallSaulD)); ReadSparkResponse();
}

void SetPresetBreezyBlues() {
  FXDriveType="Booster"; FXDriveStatus[0]=FX_OFF;
  FXModType="DigitalChorus"; FXModStatus[0]=FX_ON;
  FXDelayType="MultiHeadDelay"; FXDelayStatus[0]=FX_ON;
  FXReverbType="Chamber"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(BreezyBluesA,sizeof(BreezyBluesA)); ReadSparkResponse();
  SerialBT.write(BreezyBluesB,sizeof(BreezyBluesB)); ReadSparkResponse();
  SerialBT.write(BreezyBluesC,sizeof(BreezyBluesC)); ReadSparkResponse();
  SerialBT.write(BreezyBluesD,sizeof(BreezyBluesD)); ReadSparkResponse();
  SerialBT.write(BreezyBluesE,sizeof(BreezyBluesE)); ReadSparkResponse();
}

void SetPresetBrightTweed() {
  FXDriveType="Booster"; FXDriveStatus[0]=FX_OFF;
  FXModType="DigitalChorus"; FXModStatus[0]=FX_ON;
  FXDelayType="DigitalDelay"; FXDelayStatus[0]=FX_ON;
  FXReverbType="Chamber"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(BrightTweedA,sizeof(BrightTweedA)); ReadSparkResponse();
  SerialBT.write(BrightTweedB,sizeof(BrightTweedB)); ReadSparkResponse();
  SerialBT.write(BrightTweedC,sizeof(BrightTweedC)); ReadSparkResponse();
  SerialBT.write(BrightTweedD,sizeof(BrightTweedD)); ReadSparkResponse();
  SerialBT.write(BrightTweedE,sizeof(BrightTweedE)); ReadSparkResponse();
}


void SetPresetDancingInARoom() {
  FXDriveType="Booster"; FXDriveStatus[0]=FX_OFF;
  FXModType="DigitalChorus"; FXModStatus[0]=FX_ON;
  FXDelayType="DelayEcho"; FXDelayStatus[0]=FX_OFF;
  FXReverbType="ClassicPlate"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(DancingA,sizeof(DancingA)); ReadSparkResponse();
  SerialBT.write(DancingB,sizeof(DancingB)); ReadSparkResponse();
  SerialBT.write(DancingC,sizeof(DancingC)); ReadSparkResponse();
  SerialBT.write(DancingD,sizeof(DancingD)); ReadSparkResponse();
  SerialBT.write(DancingE,sizeof(DancingE)); ReadSparkResponse();
}


void SetPresetFuzzyJam() {
  FXDriveType="Fuzz"; FXDriveStatus[0]=FX_ON;
  FXModType="Vibe"; FXModStatus[0]=FX_OFF;
  FXDelayType="VintageDelay"; FXDelayStatus[0]=FX_OFF;
  FXReverbType="PlateShort"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(FuzzyJamA,sizeof(FuzzyJamA)); ReadSparkResponse();
  SerialBT.write(FuzzyJamB,sizeof(FuzzyJamB)); ReadSparkResponse();
  SerialBT.write(FuzzyJamC,sizeof(FuzzyJamC)); ReadSparkResponse();
  SerialBT.write(FuzzyJamD,sizeof(FuzzyJamD)); ReadSparkResponse();
}


void SetPresetHendrix() {
  FXDriveType="Fuzz"; FXDriveStatus[0]=FX_ON;
  FXModType="DigitalChorus"; FXModStatus[0]=FX_ON;
  FXDelayType="DigitalDelay"; FXDelayStatus[0]=FX_OFF;
  FXReverbType="PlateShort"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(HendrixA,sizeof(HendrixA)); ReadSparkResponse();
  SerialBT.write(HendrixB,sizeof(HendrixB)); ReadSparkResponse();
  SerialBT.write(HendrixC,sizeof(HendrixC)); ReadSparkResponse();
  SerialBT.write(HendrixD,sizeof(HendrixD)); ReadSparkResponse();
}

void SetPresetIrishOne() {
 
  FXDriveType="Booster"; FXDriveStatus[0]=FX_OFF;
  FXModType="Tremolo"; FXModStatus[0]=FX_OFF;
  FXDelayType="MultiHeadDelay"; FXDelayStatus[0]=FX_ON;
  FXReverbType="RoomStudioA"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(IrishOneA,sizeof(IrishOneA)); ReadSparkResponse();
  SerialBT.write(IrishOneB,sizeof(IrishOneB)); ReadSparkResponse();
  SerialBT.write(IrishOneC,sizeof(IrishOneC)); ReadSparkResponse();
  SerialBT.write(IrishOneD,sizeof(IrishOneD)); ReadSparkResponse();
}

void SetPresetLeFreak() {
  FXDriveType="Booster"; FXDriveStatus[0]=FX_ON;
  FXModType="Phaser"; FXModStatus[0]=FX_OFF;
  FXDelayType="EchoTape"; FXDelayStatus[0]=FX_OFF;
  FXReverbType="Chamber"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(LeFreakA,sizeof(LeFreakA)); ReadSparkResponse();
  SerialBT.write(LeFreakB,sizeof(LeFreakB)); ReadSparkResponse();
  SerialBT.write(LeFreakC,sizeof(LeFreakC)); ReadSparkResponse();
  SerialBT.write(LeFreakD,sizeof(LeFreakD)); ReadSparkResponse();
}

void SetPresetRHCP() {
  FXDriveType="Booster"; FXDriveStatus[0]=FX_OFF;
  FXModType="Tremolo"; FXModStatus[0]=FX_OFF;
  FXDelayType="DigitalDelay"; FXDelayStatus[0]=FX_OFF;
  FXReverbType="PlateShort"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(RHCPA,sizeof(RHCPA)); ReadSparkResponse();
  SerialBT.write(RHCPB,sizeof(RHCPB)); ReadSparkResponse();
  SerialBT.write(RHCPC,sizeof(RHCPC)); ReadSparkResponse();
  SerialBT.write(RHCPD,sizeof(RHCPD)); ReadSparkResponse();
}

void SetPresetSantana() {
  FXDriveType="TubeDrive"; FXDriveStatus[0]=FX_ON;
  FXModType="Tremolo"; FXModStatus[0]=FX_OFF;
  FXDelayType="DigitalDelay"; FXDelayStatus[0]=FX_OFF;
  FXReverbType="PlateShort"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(SantanaA,sizeof(SantanaA)); ReadSparkResponse();
  SerialBT.write(SantanaB,sizeof(SantanaB)); ReadSparkResponse();
  SerialBT.write(SantanaC,sizeof(SantanaC)); ReadSparkResponse();
  SerialBT.write(SantanaD,sizeof(SantanaD)); ReadSparkResponse();
}

void SetPresetSilverShip() {
  FXDriveType="Booster"; FXDriveStatus[0]=FX_OFF;
  FXModType="Cloner"; FXModStatus[0]=FX_ON; ClonerSetToLow=true;
  FXDelayType="VintageDelay"; FXDelayStatus[0]=FX_OFF;
  FXReverbType="Chamber"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(SilverShipA,sizeof(SilverShipA)); ReadSparkResponse();
  SerialBT.write(SilverShipB,sizeof(SilverShipB)); ReadSparkResponse();
  SerialBT.write(SilverShipC,sizeof(SilverShipC)); ReadSparkResponse();
  SerialBT.write(SilverShipD,sizeof(SilverShipD)); ReadSparkResponse();
}

void SetPresetSpookyMelody() {
  FXDriveType="TubeDrive"; FXDriveStatus[0]=FX_OFF;
  FXModType="Vibe"; FXModStatus[0]=FX_ON;
  FXDelayType="DelayEcho"; FXDelayStatus[0]=FX_ON;
  FXReverbType="Ambient"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(SpookyMelodyA,sizeof(SpookyMelodyA)); ReadSparkResponse();
  SerialBT.write(SpookyMelodyB,sizeof(SpookyMelodyB)); ReadSparkResponse();
  SerialBT.write(SpookyMelodyC,sizeof(SpookyMelodyC)); ReadSparkResponse();
  SerialBT.write(SpookyMelodyD,sizeof(SpookyMelodyD)); ReadSparkResponse();
  SerialBT.write(SpookyMelodyE,sizeof(SpookyMelodyE)); ReadSparkResponse();
}

void SetPresetStrayCatStrut() {
  FXDriveType="Booster"; FXDriveStatus[0]=FX_ON;
  FXModType="Tremolator"; FXModStatus[0]=FX_OFF;
  FXDelayType="EchoTape"; FXDelayStatus[0]=FX_ON;
  FXReverbType="Chamber"; FXReverbStatus[0]=FX_OFF;
  SerialBT.write(StrayCatStrutA,sizeof(StrayCatStrutA)); ReadSparkResponse();
  SerialBT.write(StrayCatStrutB,sizeof(StrayCatStrutB)); ReadSparkResponse();
  SerialBT.write(StrayCatStrutC,sizeof(StrayCatStrutC)); ReadSparkResponse();
  SerialBT.write(StrayCatStrutD,sizeof(StrayCatStrutD)); ReadSparkResponse();
}

void SetPresetSultans() {  
  FXDriveType="Booster"; FXDriveStatus[0]=FX_OFF;
  FXModType="Flanger"; FXModStatus[0]=FX_ON;
  FXDelayType="VintageDelay"; FXDelayStatus[0]=FX_ON;
  FXReverbType="Chamber"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(SultansA,sizeof(SultansA)); ReadSparkResponse();
  SerialBT.write(SultansB,sizeof(SultansB)); ReadSparkResponse();
  SerialBT.write(SultansC,sizeof(SultansC)); ReadSparkResponse();
  SerialBT.write(SultansD,sizeof(SultansD)); ReadSparkResponse();
}


void SetPresetSurf() {
  FXDriveType="Booster"; FXDriveStatus[0]=FX_ON;
  FXModType="Tremolo"; FXModStatus[0]=FX_ON;
  FXDelayType="EchoTape"; FXDelayStatus[0]=FX_OFF;
  FXReverbType="HallNatural"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(SurfA,sizeof(SurfA)); ReadSparkResponse();
  SerialBT.write(SurfB,sizeof(SurfB)); ReadSparkResponse();
  SerialBT.write(SurfC,sizeof(SurfC)); ReadSparkResponse();
  SerialBT.write(SurfD,sizeof(SurfD)); ReadSparkResponse();
}

void SetPresetWholeLottaLove() {
  FXDriveType="TubeDrive"; FXDriveStatus[0]=FX_ON;
  FXModType="Tremolo"; FXModStatus[0]=FX_OFF;
  FXDelayType="VintageDelay"; FXDelayStatus[0]=FX_ON;
  FXReverbType="RoomStudioA"; FXReverbStatus[0]=FX_ON;
  SerialBT.write(WholeLottaLoveA,sizeof(WholeLottaLoveA)); ReadSparkResponse();
  SerialBT.write(WholeLottaLoveB,sizeof(WholeLottaLoveB)); ReadSparkResponse();
  SerialBT.write(WholeLottaLoveC,sizeof(WholeLottaLoveC)); ReadSparkResponse();
  SerialBT.write(WholeLottaLoveD,sizeof(WholeLottaLoveD)); ReadSparkResponse();
}
