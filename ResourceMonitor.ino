#include "FastLED.h"

#define OutputPin 12
#define Width 16
#define Rows 4
const int LEDCount = Width*Rows;
const CRGB CPULoadColor = CRGB::Red;
const CRGB RAMColor = CRGB(128,128,128);
const CRGB GPULoadColor = CRGB::Orange;
const CRGB GPURAMColor = CRGB(128,128,128);

float OldCPU = 0;
float OldRAM = 0;
float OldGPU = 0;
float OldVRAM = 0;

float CPU = 0;
float RAM = 0;
float GPU = 0;
float VRAM = 0;

int RefreshRate = 1000;
const int MsBetweenFrames = 30; //approx 30FPS, should be plenty smooth. Even updating 10x/second that allows for 3 frames, but also allows for slower transitions between slower refresh speeds.

long LastUpdateTime = 0;


CRGB LEDStrip[LEDCount];

void setup() {
  // put your setup code here, to run once:
 Serial.begin(9600,SERIAL_8O2);
 Serial.setTimeout(5);
 Serial.println("Arduino Booted.");
 pinMode(LED_BUILTIN, OUTPUT);
 digitalWrite(LED_BUILTIN, LOW);
 FastLED.addLeds<WS2811, OutputPin, GRB>(LEDStrip, LEDCount);//.setCorrection(TypicalSMD5050);
 FastLED.setBrightness(30);
 Startup();
 LastUpdateTime = millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  if(Serial.available()>0){
    digitalWrite(LED_BUILTIN, HIGH);
    String Str = Serial.readString();
    Serial.println("Serial Received: "+Str);
    LastUpdateTime = millis();

    RefreshRate = floor(getValue(Str,',',0).toFloat()*1000);//recieve in seconds, but it's more useful in ms
    CPU = getValue(Str,',',1).toFloat();
    RAM = getValue(Str,',',2).toFloat();
    GPU = getValue(Str,',',3).toFloat();
    VRAM = getValue(Str,',',4).toFloat();

    digitalWrite(LED_BUILTIN, LOW);

    if(RefreshRate < 10 || CPU>100 || RAM>100 || GPU>100 || VRAM > 100){
      //something went wrong, and to stop ourselves from crashing, I'm just gonna throw away this transmission.
      Serial.println("Unreasonable numbers received");
      RefreshRate = 1000;
      CPU = OldCPU;
      RAM = OldRAM;
      GPU = OldGPU;
      VRAM = OldVRAM;
    } else{
      //do all the processing to interpolate between values
      Interpolate();
    }

    
  }

  if((millis()-LastUpdateTime) > (RefreshRate * 5)){//if we haven't gotten new data in 5 cycles, might as well power down.
    Serial.print("Nothing Recieved for ");
    Serial.print(RefreshRate * 5);
    Serial.println("ms. Sleeping.");
    CPU=0;
    RAM=0;
    GPU=0;
    VRAM=0;
    RefreshRate = 1000;
    //call interpolator to smooth it down to 0
    Interpolate();
    RefreshRate = 10000;
  }
}

void Interpolate(){
  int Frames = floor(RefreshRate / MsBetweenFrames);
  if(Frames < 1){
    return;
  }
  float CPUChange = (CPU - OldCPU)/Frames;
  float RAMChange = (RAM - OldRAM)/Frames;
  float GPUChange = (GPU - OldGPU)/Frames;
  float VRAMChange = (VRAM - OldVRAM)/Frames;

  long InterpolateTime = millis();
  for(int i = 1; i <Frames;i++){
    DrawGraph(OldCPU+(CPUChange*i),OldRAM+(RAMChange*i),OldGPU+(GPUChange*i),OldVRAM+(VRAMChange*i));
    delay(MsBetweenFrames-(millis()-InterpolateTime));
    InterpolateTime = millis();
  }
  
  DrawGraph(CPU,RAM,GPU,VRAM);
  //set up the values to be read for the next set of values
    OldCPU = CPU;
    OldRAM = RAM;
    OldGPU = GPU;
    OldVRAM = VRAM;
}

void DrawGraph(float CPULoad, float RAMPercent,float GPULoad, float GPURAMPercent){
  float CPUFilled = CPULoad/10*Width/10;
  float RAMFilled = RAMPercent/10*Width/10;
  float GPUFilled = GPULoad/10*Width/10;
  float VRAMFilled = GPURAMPercent/10*Width/10;
  ClearStrip();

  //CPU Load
  for(int i = 0; i < floor(CPUFilled); i++){
    AddColor(i, CPULoadColor, 1);
  }
  AddColor(floor(CPUFilled), CPULoadColor, CPUFilled-floor(CPUFilled));
  
    //RAM
  for(int i = 0; i < floor(RAMFilled); i++){
    AddColor(i+Width, RAMColor, 1);
  }
  AddColor(floor(RAMFilled)+Width, RAMColor, RAMFilled-floor(RAMFilled));
  
  //GPU Load
  for(int i = 0; i < floor(GPUFilled); i++){
    AddColor(i+(Width*2), GPULoadColor, 1);
  }
  AddColor(floor(GPUFilled)+(Width*2), GPULoadColor, GPUFilled-floor(GPUFilled));
  
  //VRAM
  for(int i = 0; i < floor(VRAMFilled); i++){
    AddColor(i+(Width*3), GPURAMColor, 1);
  }
  AddColor(floor(VRAMFilled)+(Width*3), GPURAMColor, VRAMFilled-floor(VRAMFilled));


  FastLED.show();
}

void AddColor(int index, CRGB AddColor, float Weight){
  int Red, Green, Blue;
  if(LEDStrip[index] == CRGB(0,0,0)){
    //LEDStrip[index] = AddColor;
    Red = round(AddColor.r * Weight);
    Green = round(AddColor.g * Weight);
    Blue = round(AddColor.b * Weight);
  }else{
    //LEDStrip[index].Red = (AddColor.Red * (Weight/2))+LEDStrip[index].Red;
    Red = round(((AddColor.r * Weight) + LEDStrip[index].r)/2);
    Green = round(((AddColor.g * Weight) + LEDStrip[index].g)/2);
    Blue = round(((AddColor.b * Weight) + LEDStrip[index].b)/2);
  }
  LEDStrip[index] = CRGB(Red,Green,Blue);
}

void ClearStrip(){
  for(int i = 0; i< LEDCount; i++){
    LEDStrip[i]=CRGB::Black;
  }
}

void Startup(){
  const int offset = 80;
  for(int i = 0; i<(100+offset);i++){
    DrawGraph(min(i,100), max((i-offset),0), min(i,100), max((i-offset),0));
  }
  for(int i = 100; i>(0-offset);i--){
    DrawGraph(max(i,0), min((i+offset),100), max(i,0), min((i+offset),100));
  }
  DrawGraph(0,0,0,0);
}

String getValue(String data, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = data.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (data.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}
