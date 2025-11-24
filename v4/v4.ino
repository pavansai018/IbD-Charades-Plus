#include <LiquidCrystal.h>

// LCD pin configuration
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Button pin configuration
const int buttonV = 6;
const int buttonA = 7;
const int buttonR = 8;
const int buttonK = 9;
const int buttonSkip = 10;

// LED pin configuration - using analog pins as digital
const int led1 = A0;
const int led2 = A1;
const int led3 = A2;
const int led4 = A3;

// Score variables
int scoreV = 0;
int scoreA = 0;
int scoreR = 0;
int scoreK = 0;

// Word array
const char* words[] = {
  "SUNSHINE", "MORNING", "ADVENTURE", "BEAUTIFUL", 
  "CHALLENGE", "VICTORY", "BRILLIANT", "DISCOVER",
  "FREEDOM", "HAPPINESS", "JOURNEY", "KINDNESS",
  "MIRACLE", "OPPORTUNITY", "PEACEFUL", "TREASURE",
  "WONDERFUL", "CREATIVE", "FRIENDSHIP", "INSPIRE", "VOLCANO", "AVALANCHE", "RAINBOW", "WATERFALL",
  "FOREST", "OCEAN", "MOUNTAIN", "GALAXY",
  "PLANET", "WEATHER", "CLIMATE", "ENERGY",
  "SCIENCE", "NATURE", "ANIMAL", "PLANT",
  "UNIVERSE", "ECOLOGY", "GEOLOGY", "BIOLOGY",  "SUCCESS", "ACHIEVE", "PASSION", "COURAGE",
  "DETERMINED", "POSITIVE", "GROWTH", "LEARNING",
  "STRENGTH", "WISDOM", "HARMONY", "BALANCE",
  "GRATITUDE", "PATIENCE", "RESPECT", "HONESTY",
  "CONFIDENCE", "OPTIMISM", "EMPATHY", "SERENITY",  "MYSTERY", "PUZZLE", "JOURNEY", "VICTORY",
  "FREEDOM", "WISDOM", "COURAGE", "SERENITY",
  "HARMONY", "PASSION", "LEGACY", "DESTINY",
  "FANTASY", "REALITY", "FUTURE", "HISTORY",
  "CULTURE", "TRADITION", "INNOVATION", "PROGRESS",  "PICTURE", "MUSIC", "COLOR", "NUMBER",
  "LETTER", "STORY", "DREAM", "SMILE",
  "LAUGH", "DANCE", "SING", "PLAY",
  "LEARN", "TEACH", "BUILD", "CREATE",
  "EXPLORE", "TRAVEL", "SHARE", "HELP"
};
const int wordCount = 12;

int currentWordIndex = -1;
int currentLed = -1; // Which LED is currently lit

// Button state tracking
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 300;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  delay(500);
  // Initialize buttons
  pinMode(buttonV, INPUT_PULLUP);
  pinMode(buttonA, INPUT_PULLUP);
  pinMode(buttonR, INPUT_PULLUP);
  pinMode(buttonK, INPUT_PULLUP);
  pinMode(buttonSkip, INPUT_PULLUP);
  
  // Initialize LEDs
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  
  // Turn off all LEDs initially
  turnOffAllLEDs();
  delay(500);
  randomSeed(analogRead(A5)); // Use unused analog pin for random seed
  delay(500);
  Serial.println("=== SYSTEM STARTED ===");
  Serial.println("LED to Button mapping:");
  Serial.println("LED1 -> Button V");
  Serial.println("LED2 -> Button A"); 
  Serial.println("LED3 -> Button R");
  Serial.println("LED4 -> Button K");
  
  displayRandomWord();
  lightRandomLED();
  updateScoreDisplay();
  Serial.println("Ready for button presses!");
}

void loop() {
  // Simple direct button checking
  checkButtonSimple(buttonV, 'V');
  checkButtonSimple(buttonA, 'A');
  checkButtonSimple(buttonR, 'R');
  checkButtonSimple(buttonK, 'K');
  checkButtonSimple(buttonSkip, 'S');
}

void checkButtonSimple(int buttonPin, char buttonType) {
  // Read button state directly
  int buttonState = digitalRead(buttonPin);
  
  // With INPUT_PULLUP: LOW means pressed
  if (buttonState == LOW) {
    Serial.print("=== BUTTON ");
    Serial.print(buttonType);
    Serial.println(" PRESSED ===");
    
    // Debounce check
    if (millis() - lastButtonPress > debounceDelay) {
      handleButtonPress(buttonType);
      lastButtonPress = millis();
    }
    
    // Wait for button release
    while (digitalRead(buttonPin) == LOW) {
      delay(10);
    }
    Serial.print("Button ");
    Serial.print(buttonType);
    Serial.println(" released");
  }
}

void handleButtonPress(char buttonType) {
  // Check if correct button was pressed for the lit LED
  bool correctPress = checkLEDButtonMatch(buttonType);
  
  // Update the score based on whether it was correct
  if (correctPress) {
    switch(buttonType) {
      case 'V': 
        scoreV++;
        Serial.println(">>> CORRECT! V score incremented!");
        break;
      case 'A': 
        scoreA++;
        Serial.println(">>> CORRECT! A score incremented!");
        break;
      case 'R': 
        scoreR++;
        Serial.println(">>> CORRECT! R score incremented!");
        break;
      case 'K': 
        scoreK++;
        Serial.println(">>> CORRECT! K score incremented!");
        break;
      case 'S': 
        Serial.println(">>> Skip pressed - no score change");
        break;
    }
  } else {
    Serial.println(">>> WRONG button pressed! No score change.");
  }
  
  // Turn off current LED
  turnOffAllLEDs();
  
  // Update displays and show new word with new LED
  updateScoreDisplay();
  displayRandomWord();
  lightRandomLED();
  
  Serial.println("--- Ready for next button ---");
}

bool checkLEDButtonMatch(char buttonType) {
  // Check if pressed button matches the lit LED
  // LED1 -> V, LED2 -> A, LED3 -> R, LED4 -> K
  
  bool isMatch = false;
  
  switch(buttonType) {
    case 'V':
      isMatch = (currentLed == 0); // LED1 corresponds to V
      break;
    case 'A':
      isMatch = (currentLed == 1); // LED2 corresponds to A
      break;
    case 'R':
      isMatch = (currentLed == 2); // LED3 corresponds to R
      break;
    case 'K':
      isMatch = (currentLed == 3); // LED4 corresponds to K
      break;
    case 'S':
      isMatch = true; // Skip is always valid
      break;
  }
  
  Serial.print("LED ");
  Serial.print(currentLed + 1);
  Serial.print(" lit, Button ");
  Serial.print(buttonType);
  Serial.print(" pressed - ");
  Serial.println(isMatch ? "MATCH!" : "NO MATCH!");
  
  return isMatch;
}

void lightRandomLED() {
  // Turn off all LEDs first
  turnOffAllLEDs();
  
  // Randomly select an LED (0-3)
  currentLed = random(4);
  
  // Light up the selected LED
  switch(currentLed) {
    case 0:
      digitalWrite(led1, HIGH);
      Serial.println(">>> LED1 lit (Press V)");
      break;
    case 1:
      digitalWrite(led2, HIGH);
      Serial.println(">>> LED2 lit (Press A)");
      break;
    case 2:
      digitalWrite(led3, HIGH);
      Serial.println(">>> LED3 lit (Press R)");
      break;
    case 3:
      digitalWrite(led4, HIGH);
      Serial.println(">>> LED4 lit (Press K)");
      break;
  }
}

void turnOffAllLEDs() {
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
  digitalWrite(led4, LOW);
}

void displayRandomWord() {
  int newIndex;
  do {
    newIndex = random(wordCount);
  } while (newIndex == currentWordIndex && wordCount > 1);
  
  currentWordIndex = newIndex;
  
  lcd.setCursor(0, 0);
  lcd.print("                "); // Clear line
  lcd.setCursor(0, 0);
  lcd.print(words[currentWordIndex]);
  
  Serial.print("New word displayed: ");
  Serial.println(words[currentWordIndex]);
}

void updateScoreDisplay() {
  // Clear the entire second line first
  lcd.setCursor(0, 1);
  lcd.print("                "); // Clear entire line
  delay(50);
  // Optimized spacing to fit all scores within 16 characters
  // Format: "V:99 A:99 R:99 K:99" = 15 characters total
  
  // V score (positions 0-3)
  lcd.setCursor(0, 1);
  delay(50);
  lcd.print("V");
  printScore(scoreV, 2); // 2 characters for V score
  delay(50);
//  lcd.print("*");
  // A score (positions 4-7)  
  lcd.setCursor(4, 1);
  delay(50);
  lcd.print("A");
  delay(50);
  printScore(scoreA, 2); // 2 characters for A score
//  lcd.print("*");
  // R score (positions 8-11)
  lcd.setCursor(8, 1);
  delay(50);
  lcd.print("R");
  delay(50);
  printScore(scoreR, 2); // 2 characters for R score
//  lcd.print("*");
  // K score (positions 12-15) - LAST 4 CHARACTERS
  lcd.setCursor(12, 1);
  delay(50);
  lcd.print("K");
  delay(50);
  printScore(scoreK, 2); // 2 characters for K score
//  lcd.print("*");
  delay(50);
  // Update Serial for debugging
  Serial.print("Current scores - V:");
  Serial.print(scoreV);
  Serial.print(" A:");
  Serial.print(scoreA);
  Serial.print(" R:");
  Serial.print(scoreR);
  Serial.print(" K:");
  Serial.println(scoreK);
}

void printTwoDigitScore(int score) {
  if (score < 10) {
    lcd.print("0");
    lcd.print(score);
  } else if (score < 100) {
    lcd.print(score);
  } else {
    lcd.print("99");
  }
}
// Helper function to print scores with proper formatting
void printScore(int score, int width) {
  if (score < 10) {
    lcd.print(" "); // Add space for single digit numbers
    lcd.print(score);
  } else if (score < 100) {
    lcd.print(score); // Two digit numbers fit perfectly
  } else {
    lcd.print("99"); // Limit to 99 if exceeds
  }
}

void logButtonStates() {
  Serial.print("Button states: V=");
  Serial.print(digitalRead(buttonV));
  Serial.print(" A=");
  Serial.print(digitalRead(buttonA));
  Serial.print(" R=");
  Serial.print(digitalRead(buttonR));
  Serial.print(" K=");
  Serial.print(digitalRead(buttonK));
  Serial.print(" Skip=");
  Serial.println(digitalRead(buttonSkip));
}
