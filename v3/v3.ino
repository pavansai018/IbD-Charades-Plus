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

// Score variables
int scoreV = 0;
int scoreA = 0;
int scoreR = 0;
int scoreK = 0;

// Word array
const char* words[] = {
  "EXAMPLE", "PROJECT", "ARDUINO", "DISPLAY", 
  "BUTTONS", "SCORING", "PROGRAM", "ELECTRON",
  "CIRCUIT", "SENSOR", "DIGITAL", "ANALOG"
};
const int wordCount = 12;

int currentWordIndex = -1;

// Button state tracking
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 300;

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  
  // Initialize buttons
  pinMode(buttonV, INPUT_PULLUP);
  pinMode(buttonA, INPUT_PULLUP);
  pinMode(buttonR, INPUT_PULLUP);
  pinMode(buttonK, INPUT_PULLUP);
  pinMode(buttonSkip, INPUT_PULLUP);
  
  randomSeed(analogRead(0));
  
  Serial.println("=== SYSTEM STARTED ===");
  Serial.println("All buttons should show 1 (HIGH) when not pressed");
  logButtonStates();
  
  displayRandomWord();
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
  // Update the score
  switch(buttonType) {
    case 'V': 
      scoreV++;
      Serial.println(">>> V score incremented!");
      break;
    case 'A': 
      scoreA++;
      Serial.println(">>> A score incremented!");
      break;
    case 'R': 
      scoreR++;
      Serial.println(">>> R score incremented!");
      break;
    case 'K': 
      scoreK++;
      Serial.println(">>> K score incremented!");
      break;
    case 'S': 
      Serial.println(">>> Skip pressed - no score change");
      break;
  }
  
  // Update displays
  updateScoreDisplay();
  displayRandomWord();
  
  Serial.println("--- Ready for next button ---");
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

// FIXED: Proper score display with correct spacing
void updateScoreDisplay() {
  // Clear the entire second line first
  lcd.setCursor(0, 1);
  lcd.print("                "); // Clear entire line
  
  // Optimized spacing to fit all scores within 16 characters
  // Format: "V:99 A:99 R:99 K:99" = 15 characters total
  
  // V score (positions 0-3)
  lcd.setCursor(0, 1);
  lcd.print("V");
  printScore(scoreV, 2); // 2 characters for V score
//  lcd.print("*");
  // A score (positions 4-7)  
  lcd.setCursor(4, 1);
  lcd.print("A");
  printScore(scoreA, 2); // 2 characters for A score
//  lcd.print("*");
  // R score (positions 8-11)
  lcd.setCursor(8, 1);
  lcd.print("R");
  printScore(scoreR, 2); // 2 characters for R score
//  lcd.print("*");
  // K score (positions 12-15) - LAST 4 CHARACTERS
  lcd.setCursor(12, 1);
  lcd.print("K");
  printScore(scoreK, 2); // 2 characters for K score
//  lcd.print("*");
  
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

// ALTERNATIVE: Even more compact version (if above doesn't work)
void updateScoreDisplayCompact() {
  // Clear the entire second line first
  lcd.setCursor(0, 1);
  lcd.print("                "); // Clear entire line
  
  // Super compact: "V:9 A:9 R:9 K:9" - single digit scores
  // But we'll handle two digits by being smart about spacing
  
  lcd.setCursor(0, 1);
  lcd.print("V");
  printCompactScore(scoreV, 1);
  
  lcd.setCursor(4, 1);
  lcd.print("A");
  printCompactScore(scoreA, 5);
  
  lcd.setCursor(8, 1);
  lcd.print("R");
  printCompactScore(scoreR, 9);
  
  lcd.setCursor(12, 1);
  lcd.print("K");
  printCompactScore(scoreK, 13);
}

void printCompactScore(int score, int basePos) {
  lcd.setCursor(basePos, 1);
  lcd.print(":");
  if (score < 10) {
    lcd.print(score);
    lcd.print(" "); // Add space if single digit
  } else {
    lcd.print(score);
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
