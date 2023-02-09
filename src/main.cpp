#include <Arduino.h>
#include <MIDIUSB.h>
#include <Adafruit_NeoPixel.h>
const int neopixelPin = 10;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(16, neopixelPin, NEO_GRB + NEO_KHZ800);
//ADD MIDIUSB LIBRARY VIA LIBRARY MANAGER
//added hotSwap mode (red btn on boot) swaps between notemode and cc

int animBrightness = 0;
int animState = 1;
int previousButtonState[8] = {HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH, HIGH};   // for checking the state of a pushButton
bool activatedKnobs[8] = {false, false, false, false, true, true, true, true};
int buttPins[8] = {2, 3, 5, 7, 14, 15, 16, 0};
int knobPins[8] = {A0,A1,A2,A3,A6,A7,A8,A9};
int notes[8] = {67, 65, 66, 64, 68, 69, 70, 71};
int knobCCs[8] = {10,11,12,13,14,15,16,17};
unsigned long debounce[8] {0, 0, 0, 0, 0, 0, 0, 0};
int debounceT = 10;
int oldKnobs[8] = {0,0,0,0,0,0,0,0};

bool noteMode = true; //mode to store Notes or CC


void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}


// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
}
int diffThresh = 10;
void handleKnobs(){
  for(int i = 0; i<8; i++){
    int thisKnob = analogRead(knobPins[i]);
    if(thisKnob < oldKnobs[i] - diffThresh | thisKnob > oldKnobs[i] + diffThresh ){
      oldKnobs[i] = thisKnob;
      int midiVal = thisKnob>>3;
      //Serial.print("CC" + i);
      if(activatedKnobs[i]){
        //Serial.println(midiVal);
        controlChange(0,knobCCs[i],midiVal);  
      }
    }
  }
  MidiUSB.flush();
}

  
void handleButtons(){
  for (byte i = 0; i < 7; i++) {
    if (debounce[i] < millis()) {
      bool buttonState = !digitalRead(buttPins[i]);
      if(!buttonState){

      }
      if (buttonState && !previousButtonState[i]) {
        debounce[i] = millis() + debounceT;
        animState = i%4;
        animBrightness = 255;
        if(noteMode){
          noteOn(0, notes[i], 64);   // Channel 0, middle C, normal velocity  
        } else {
          controlChange(0,127,notes[i]);
        }
        MidiUSB.flush();
      }
      if (!buttonState && previousButtonState[i]) {
        debounce[i] = millis() + debounceT;
        if(noteMode){
          noteOff(0, notes[i], 64);   // Channel 0, middle C, normal velocity  
        } else {
          controlChange(0,0,notes[i]);
        }
        
        MidiUSB.flush();
      }
      previousButtonState[i] = buttonState;
    }
  }
}

uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

int spinnerMember = 0;
long unsigned int spinnerTimer = 0;
void spinner(){
int spinnerSpeed = analogRead(knobPins[7]);
spinnerSpeed = map(spinnerSpeed,0,1024,150,10);

Serial.println(spinnerSpeed);
if(millis()>spinnerTimer+spinnerSpeed){
  spinnerTimer = millis();
  spinnerMember++;
  spinnerMember = spinnerMember % strip.numPixels();
  strip.clear();
  strip.setPixelColor(spinnerMember,strip.Color(127, 127, 127));
  strip.show();
  }
}

int rainbowCounter = 0;
void rainbowCycle() {
  rainbowCounter ++;
  //rainbowCounter = rainbowCounter % 256;
  uint16_t i;
    for(i=0; i< strip.numPixels(); i++) {
      strip.setBrightness(animBrightness);
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + rainbowCounter) & 255));
    }
    animBrightness-=2;
    if(animBrightness < 2){
      animBrightness = 2;
    }
    strip.show();
}
int chaseCounter = 0;
unsigned long chaseCounterTimer = 0;
void theaterChase(uint32_t c) {
  if(millis()>chaseCounterTimer+1){
    strip.clear();
    for(int i = 0; i<strip.numPixels(); i++){
      strip.setPixelColor(i, c);    //turn every pixel on
    }
    //strip.setPixelColor(random(0,16), c);    //turn every third pixel on
    strip.setBrightness(animBrightness);
    strip.show();
    chaseCounterTimer = millis();
    
    if(animBrightness < 4){
      animBrightness = 57;
      
      strip.setBrightness(animBrightness);
      animState = 8;
    } else {
      animBrightness-=2;
    }
  }
  
}

void setup() {
  // make the pushButton pin an input:
  for (byte i = 0; i < 8; i++) {
    pinMode(buttPins[i], INPUT_PULLUP);
    pinMode(knobPins[i], INPUT);
  }
  strip.begin();
  strip.setBrightness(50);
  strip.show(); // Initialize all pixels to 'off'
  
}

void handleNeoPixels(){
    switch(animState){
    case 0:
      theaterChase(strip.Color(127, 127, 0)); // White      
      break;
    case 1:
      theaterChase(strip.Color(127, 0, 0)); // White
      break;
    case 2:
    theaterChase(strip.Color(127, 127, 127)); // White
      break;
    case 3:
    theaterChase(strip.Color(0, 127, 127)); // White
      break;    
    case 4:
    theaterChase(strip.Color(0, 127, 0)); // White
      break;
      case 5:
    theaterChase(strip.Color(127, 0, 127)); // White
      break;
      case 6:
    theaterChase(strip.Color(30, 127, 50)); // White
      break;
      case 7:
    theaterChase(strip.Color(127, 40, 40)); // White
      break;    
    case 8:
    spinner();
      break;    
    default:
      break;
  }
}

void loop() {
  handleButtons();
  handleKnobs();
  handleNeoPixels();
}

