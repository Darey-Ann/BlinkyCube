#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include "Adafruit_MPR121.h"

#ifndef _BV
#define _BV(bit) (1 << (bit)) 
#endif

Adafruit_MPR121 cap = Adafruit_MPR121();

int maxlevel = 7;  // maxlevel 7 means 3 levels: 4,5,6
int sequence[7];  // Larger array to hold the sequence

const int NEOPIXEL_PIN = A0;   // Pin for NeoPixel strip
const int NUM_LEDS = 21;       // Total LEDs
const int CENTER_LED = 20;     // The center LED (last one in the strip, index 20)

const int REDPIN = 0;    // Pin for RED 
const int YELLOWPIN = 2; // Pin for YELLOW 
const int GREENPIN = 4;  // Pin for GREEN 
const int BLUEPIN = 6;   // Pin for BLUE 
const int GAME = 8;   // Pin for GAME

const int PIEZO = 11;   // Pin for PIEZO

// colors in order
String colors[] = {"Red", "Yellow", "Green", "Blue"}; 

Adafruit_NeoPixel strip(NUM_LEDS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

uint16_t lasttouched = 0;
uint16_t currtouched = 0;

// Quadrant LED indexes 
const int quadrantRed[] = {6, 7, 8, 18, 19};    
const int quadrantGreen[] = {0, 1, 2, 12, 13, 16};   // power source
const int quadrantBlue[] = {3, 4, 5, 14, 15, 17}; 
const int quadrantYellow[] = {9, 10, 11, 20};        

// Predefined color values for each button
const int redColor[] = {255, 30, 10};     // Red: RGB
const int yellowColor[] = {255, 225, 0};  // Yellow: RGB
const int greenColor[] = {30, 255, 0};    // Green: RGB
const int blueColor[] = {30, 100, 255};   // Blue: RGB

const int redCenterColor[] = {255, 100, 50};
const int yellowCenterColor[] = {255, 220, 100};
const int greenCenterColor[] = {100, 255, 150};
const int blueCenterColor[] = {100, 180, 255};


//int sequence[4]; // Array to hold the random sequence indices

//int values[4]; // Array to hold the actual random sequence
int playerIndex = 0; // Tracks the player's progress in the sequence
bool gameActive = false; // Tracks whether the game is active

unsigned long lastInteractionTime = 0;
bool idleMode = false;
bool gameCompleted = false;  

int sequenceSize = 4;  // Start with an initial size of 4 (the first 4 elements)


void setup() {
  Serial.begin(9600);
  while (!Serial) { delay(10); }

  Serial.println("Adafruit MPR121 Capacitive Touch sensor test");

  if (!cap.begin(0x5A)) {
    Serial.println("MPR121 not found, check wiring?");
    while (1);
  }
  Serial.println("MPR121 found!");

  cap.setThresholds(3, 3); // Adjust sensitivity

  pinMode(PIEZO, OUTPUT);
  strip.begin();
  strip.show(); // Initialize all pixels to off
}

void loop() {
  currtouched = cap.touched(); // Read touch state

  if (currtouched) {
    lastInteractionTime = millis(); // Reset idle timer
    if (idleMode) {
      idleMode = false;
      strip.clear();
      strip.show();
    }
  }

  if (millis() - lastInteractionTime > 60000) { // 1-minute idle time = 60000
    if (!idleMode) {
      idleMode = true;
    }
    pulsatingAmbientLight();
    return; // Skip the rest of the loop
  }

  strip.clear(); // Clear LEDs before updating
  // Check if the sequence button is pressed
  if ((currtouched & _BV(GAME)) && !(lasttouched & _BV(GAME))) {
    if (!gameActive) {
      sequenceSize = 4;  // Reset sequence length
      gameCompleted = false; 
      Serial.println();
      Serial.println(".............................................");
      Serial.println("Sequence button pressed! Starting the game...");
      Serial.println(".............................................");
      Serial.println();
      generateRandomSequence();
      playGamestart();
      delay(300);
      playSequence();
      gameActive = true;
      playerIndex = 0;
    }
  } else if (gameActive) {
    if (checkPlayerInput()) {
      Serial.println("*******************test *********");
      Serial.println("****************************");
      Serial.println("*** ^.^ W I N N E R ! ^.^***");
      Serial.println("****************************");
      Serial.println("****************************");
      Serial.println();
      playCelebrationSequence();
      gameActive = false;
      gameCompleted = true;

      // attempted logic to add levels
      //int values[] = {0, 2, 4, 6};  // Possible values
      //int randomIndex = random(0, 4);  // Generate a random index (0 to 3)
      //appendToSequence(values[randomIndex]);  // Append the new value
      //playSequence();

      // Reset sequenceSize and ensure game doesn't restart immediately
      sequenceSize = 4;  
      return;  // Stop execution to prevent the sequence from playing again
    }
  } else {
    checkIndividualButtons();
  }

  lasttouched = currtouched;
  delay(10);
}


void checkIndividualButtons() {
  // Check the RED button (0)
  if ((currtouched & _BV(REDPIN)) && !(lasttouched & _BV(REDPIN))) {
    Serial.println("RED button pressed! A note played!");
    lightQuadrant(REDPIN); // Red (blended center)
    tone(PIEZO, 220); // A (220 Hz)
  }
  // Check the YELLOW button (2)
  if ((currtouched & _BV(YELLOWPIN)) && !(lasttouched & _BV(YELLOWPIN))) {
    Serial.println("YELLOW button pressed! B note played!");
    lightQuadrant(YELLOWPIN); // Yellow
    tone(PIEZO, 247); // B (247 Hz)
  }
  // Check the GREEN button (4)
  if ((currtouched & _BV(GREENPIN)) && !(lasttouched & _BV(GREENPIN))) {
    Serial.println("GREEN button pressed! C note played!");
    lightQuadrant(GREENPIN); // Green
    tone(PIEZO, 277); // C (277 Hz)
  }
  if ((currtouched & _BV(BLUEPIN)) && !(lasttouched & _BV(BLUEPIN))) {
    Serial.println("BLUE button pressed! D note played!");
    lightQuadrant(BLUEPIN); // Blue
    tone(PIEZO, 293); // D (293 Hz)
  }
  // If no touch is detected, turn off LEDs and stop the tone
  if (currtouched == 0 && lasttouched != 0) {
    strip.clear();
    strip.show();
    noTone(PIEZO); // Stop the sound when touch is released
  }
  lasttouched = currtouched;
  delay(10); // Small delay for debounce
}


// --- Generate random numbers between 2 and 5 (LED pins)

// This one with the seed didn't provide enough variation
// ---
//void generateRandomSequence() {
//  for (int i = 0; i < 4; i++) {
//    sequence[i] = random(2, 6); 
//  }
//}
// ---

// use algorithm for variation instead to generate a random sequence with better variation
int values[] = {0, 2, 4, 6};  // Possible values


void generateRandomSequence() {
  randomSeed(millis()); // Ensures randomness at startup
  // Generate an initial sequence of 4 random numbers between 2 and 5
  for (int i = 0; i < sequenceSize; i++) {
    int randomIndex = random(0, 4);  // Generate a random index (0 to 3)
    sequence[i] = values[randomIndex];
  }

  // Shuffle the sequence to ensure more variation
 // shuffleSequence(sequence, 4); // Shuffle the array of 4 elements
}

// Shuffle function using the Fisher-Yates algorithm
void shuffleSequence(int arr[], int n) {
  for (int i = n - 1; i > 0; i--) {
    int j = random(0, i + 1);  // Pick a random index to swap with
    // Swap elements
    int temp = arr[i];
    arr[i] = arr[j];
    arr[j] = temp;
  }
}


void appendToSequence() {
  if (sequenceSize < sizeof(sequence) / sizeof(sequence[0]) - 1) {
    int randomIndex = random(0, 4);
    sequence[sequenceSize] = values[randomIndex];
    sequenceSize++;
  } else {
    playFinalBoss();
    Serial.println("Max level reached!");
    Serial.println("****************************");
    Serial.println("****************************");
    Serial.println("*** ^.^ W I N N E R ! ^.^***");
    Serial.println("****************************");
    Serial.println("****************************");
    Serial.println("*** GAME OVER ***");


    gameActive = false;
    gameCompleted = true;  // Mark the game as completed
  }
}




void playGamestart() {
  int melody[]   = {220, 277, 293};
  int duration[] = {90, 90, 450};

  delay(300);

  // Turn on the NeoPixels with a mix of warm and cool white tones
  for (int i = 0; i < 21; i++) {
    int temp = i % 3; // Cycle through different white tones
  
    uint32_t color;
    if (temp == 0) {
      color = strip.Color(180, 175, 160);  // Dim warm white
    } else if (temp == 1) {
      color = strip.Color(200, 200, 200);  // Dim pure white
    } else {
      color = strip.Color(170, 180, 200);  // Dim cool white
    }
  
    strip.setPixelColor(i, strip.gamma32(color));
  }
  strip.show();


  // Play the melody
  for (int i = 0; i < 3; i++) { // if i use sizeof(melody) the piezo makes a weird buzzing and then high pitched sound..??
    tone(PIEZO, melody[i], duration[i]);
    delay(duration[i] + 50);  // Wait for the note to finish, with a small gap
  }
}

void playSequence() {
  Serial.println("Cheat sheet: ");
  
  for (int i = 0; i < sequenceSize; i++) {
    turnOffAllLEDs();
    delay(500);
    int delaybetweennotes = 400;

    int ledPin = sequence[i]; // Get the current step in the sequence

    Serial.print(colors[ledPin / 2]); // Print the color
    Serial.print("-");

    // Light up the correct LED and play the corresponding tone
    if (ledPin == REDPIN) {
      lightQuadrant(REDPIN); // Red (blended center)
      tone(PIEZO, 220); // A (220 Hz)
      delay(delaybetweennotes);
    } 
    else if (ledPin == YELLOWPIN) {
      lightQuadrant(YELLOWPIN); // Yellow
      tone(PIEZO, 247); // B (247 Hz)
      delay(delaybetweennotes);
    } 
    else if (ledPin == GREENPIN) {
      lightQuadrant(GREENPIN); // Green
      tone(PIEZO, 277); // C (277 Hz)
      delay(delaybetweennotes);
    } 
    else if (ledPin == BLUEPIN) {
      lightQuadrant(BLUEPIN); // Blue
      tone(PIEZO, 293); // D (293 Hz)
      delay(delaybetweennotes);
    }

    delay(20); // Delay for the note duration
    noTone(PIEZO); // Stop the tone
  }

  turnOffAllLEDs(); // Ensure all LEDs are off after playback
  Serial.println("- Now it's your turn...");
}






void lightQuadrant(int pin) {
  int *quadrant;
  const int *color;
  const int *centerColor;
  int size;

  // Determine the quadrant, color, and center color based on the pin
  switch (pin) {
    case REDPIN:
      quadrant = quadrantRed;
      color = redColor;
      centerColor = redCenterColor;
      size = 6;
      break;
    case YELLOWPIN:
      quadrant = quadrantYellow;
      color = yellowColor;
      centerColor = yellowCenterColor;
      size = 6;
      break;
    case GREENPIN:
      quadrant = quadrantGreen;
      color = greenColor;
      centerColor = greenCenterColor;
      size = 6;
      break;
    case BLUEPIN:
      quadrant = quadrantBlue;
      color = blueColor;
      centerColor = blueCenterColor;
      size = 6;
      break;
    default:
      return; // If the pin doesn't match any case, do nothing
  }

  // Light up the quadrant with the specified color
  for (int i = 0; i < size; i++) {
    strip.setPixelColor(quadrant[i], strip.Color(color[0], color[1], color[2]));
  }
  
  // Set the center LED with a slightly blended color
  strip.setPixelColor(CENTER_LED, strip.Color(centerColor[0], centerColor[1], centerColor[2]));
  
  strip.show();
}



// ---- PLAYER INPUT LOGIC
bool checkPlayerInput() {
  int touchPads[] = {0, 2, 4, 6};

  for (int i = 0; i < sequenceSize; i++) {
    // Check if the specific touchpad is pressed (new press)
    if ((currtouched & _BV(touchPads[i])) && !(lasttouched & _BV(touchPads[i]))) {
      
      // Check if the pressed button matches the expected one in the sequence
      if (touchPads[i] == sequence[playerIndex]) { 
        playNote(touchPads[i]);
        //lightQuadrant(touchPads[i]);  // Light up the corresponding LED
        Serial.print("You pressed: ");
        Serial.println(colors[touchPads[i] / 2]); // Output the correct color
        playerIndex++;

        // If the player reaches the end of the sequence, they win
        if (playerIndex >= sequenceSize) {
          Serial.println("");
          Serial.println("*****************");
          Serial.println("*** LEVEL UP! ***");
          Serial.println("*****************");
          Serial.print("Sequence Size ");
          Serial.print(sequenceSize+1);
          Serial.println("");

          playCelebrationSequence();
          appendToSequence();  // Add new step
          delay(500);
          
          playerIndex = 0;

          if (gameActive) {
            playSequence();  // Replay updated sequence
          }
          //return true;
          
          
        }
      } else {
        // Wrong button pressed
        Serial.print("You pressed: ");
        Serial.print(colors[touchPads[i] / 2]);
        Serial.print(" | Expected: ");
        Serial.println(colors[sequence[playerIndex] / 2]);

        Serial.println("... Wrong button");
        Serial.println("---------------------------");
        Serial.println("---------------------------");
        Serial.println("--- T R Y  A G A I N :( ---");
        Serial.println("---------------------------");
        Serial.println("---------------------------");
        Serial.println();

        playLoserMelody();
        gameActive = false; 
        sequenceSize = 4;  // Reset to base length
        playerIndex = 0; // Reset sequence progress
        return false; // Stop checking other inputs
      }

      delay(300); // Prevent accidental multi-presses
      return false; // Continue checking sequence
    }
  }
  return false; // No correct input detected
}


void playNote(int ledPin) {
  lightQuadrant(ledPin);  // Light up LED
  int toneFrequency = 0;

  switch (ledPin) {
    case REDPIN: toneFrequency = 220; break;
    case YELLOWPIN: toneFrequency = 247; break;
    case GREENPIN: toneFrequency = 277; break;
    case BLUEPIN: toneFrequency = 293; break;
  }

  if (toneFrequency > 0) {
    tone(PIEZO, toneFrequency, 300);
    delay(300); // Allow time for the tone
  }

  strip.clear(); // Ensure the LED turns off after input
  strip.show();
  noTone(PIEZO);
}




void playCelebrationSequence() {
  delay(300);
  int melody[]   = {220, 247, 277, 293, 277, 247, 220};
  int duration[] = {170, 170, 170, 170, 170, 170, 400};

  // Turn on the NeoPixels with a full rainbow gradient
  for (int i = 0; i < 21; i++) {
    //strip.setPixelColor(i, strip.gamma32(strip.ColorHSV((i * 65536L / 21), 255, 255)));
    // more vibrant rainbow
    uint16_t hue = (i * 65536L) / strip.numPixels(); // Spread hue across all pixels
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue, 255, 255))); // Apply color with full saturation and brightness
  }
  strip.show();

  // Play the melody
  for (int i = 0; i < 7; i++) {
    tone(PIEZO, melody[i]);
    delay(duration[i]);
  }

  // Turn everything off
  noTone(PIEZO);
  turnOffAllLEDs();
}

void playFinalBoss() {
  delay(100);
  int melody[]   = {220, 0,   220, 277, 329, 440, 329, 440};
  int duration[] = {400, 100, 170, 170, 170, 350, 170, 600};

  // Turn on the NeoPixels with a full rainbow gradient
  for (int i = 0; i < 21; i++) {
    //strip.setPixelColor(i, strip.gamma32(strip.ColorHSV((i * 65536L / 21), 255, 255)));
    // more vibrant rainbow
    uint16_t hue = (i * 65536L) / strip.numPixels(); // Spread hue across all pixels
    strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(hue, 255, 255))); // Apply color with full saturation and brightness
  }
  strip.show();

  // Play the melody
  for (int i = 0; i < 8; i++) {
    tone(PIEZO, melody[i]);
    delay(duration[i]);
  }

  // Turn everything off
  noTone(PIEZO);
  turnOffAllLEDs();
}


void showRainbowEffect() {
  // Create a smooth rainbow transition across the LEDs
  for (int i = 0; i < NUM_LEDS; i++) {
    int color = Wheel((i + millis() / 10) & 255);
    strip.setPixelColor(i, color);
  }
  strip.show();
  delay(50); // Slow down the effect
}

// Generate a color from the wheel (used for rainbow effect)
int Wheel(byte WheelPos) {
  if (WheelPos < 85) {
    return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  } else {
    WheelPos -= 170;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
}


void pulsatingAmbientLight() {
  static int brightness = 50; // Keep a minimum brightness
  static int fadeAmount = 5;
  static int colorIndex = 0;

  for (int i = 0; i < NUM_LEDS; i++) {
    uint32_t color = Wheel((colorIndex + i * 10) & 255);
    strip.setPixelColor(i, dimColor(color, brightness));
  }
  strip.show();

  brightness += fadeAmount;
  if (brightness <= 50 || brightness >= 150) {
    fadeAmount = -fadeAmount;
  }
  
  colorIndex += 3; // Slow, smooth color shifts
  delay(50);
}

uint32_t dimColor(uint32_t color, int brightness) {
  byte r = (color >> 16) & 0xFF;
  byte g = (color >> 8) & 0xFF;
  byte b = color & 0xFF;
  return strip.Color(r * brightness / 255, g * brightness / 255, b * brightness / 255);
}




void turnOffAllLEDs() {
  // Turn off all LEDs
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, 0);
  }
  strip.show();
}

void turnOnLED(int ledPin) {
  digitalWrite(ledPin, HIGH);
}



void playLoserMelody() {
  int melody[]   = {220, 0  , 220};  // A (pause), AAA (longer)
  int duration[] = {80, 10, 600};  // short, pause, long

  delay(200);

  // Set the NeoPixels to a soft red (mixing red with a little green and blue)
  for (int i = 0; i < NUM_LEDS; i++) {
    strip.setPixelColor(i, strip.Color(100, 30, 10)); // Soft red color
  }
  strip.show();
  delay(50); // Slow down the effect

  for (int i = 0; i < 3; i++) {
    tone(PIEZO, melody[i], duration[i]);
    delay(duration[i] + 50);
  }
  turnOffAllLEDs();
}


