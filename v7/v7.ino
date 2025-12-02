#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SD.h>

// LCD configuration for I2C - Changed to 20x4
LiquidCrystal_I2C lcd(0x27, 20, 4);

// SD Card - Hardware SPI (Standard pins)
const int SD_CHIP_SELECT = 4;    // CS pin (your connection)
// Note: Hardware SPI uses fixed pins:
// MOSI = Pin 11
// MISO = Pin 12  
// SCK = Pin 13

// Button pin configuration
const int buttonV = 6;
const int buttonA = 7;
const int buttonR = 8;
const int buttonK = 9;
const int buttonSkip = 10;

// Navigation buttons
const int buttonLeft = A3;
const int buttonRight = A4;
const int buttonSelect = A5;

// LED pin configuration
const int led1 = A0;
const int led2 = A1;
const int led3 = A2;
const int led4 = 2;

// Game state
byte gameState = 0; // 0=player menu, 1=theme menu, 2=playing, 3=winner screen, 4=scores screen
byte selectedPlayers = 1;
byte currentThemeIndex = 0;
unsigned long usedWords = 0;
byte currentLed = 0;
byte currentPlayer = 0;

// LED state tracking
bool ledLit = false;

// Scores screen navigation
byte scoresScreenPage = 0;

// SD Card theme management
char themeNames[20][13];  // Store theme filenames (8.3 format + null)
byte totalThemes = 0;
char currentWords[20][13]; // Store words for current theme (max 12 chars + null)
byte totalWordsInTheme = 0;

// Individual mode scores for each player
int playerScores[4][4] = {
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0},
  {0, 0, 0, 0}
};

// Button state tracking
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 300;

void setup() {
  Serial.begin(9600);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  // Initialize buttons
  pinMode(buttonV, INPUT_PULLUP);
  pinMode(buttonA, INPUT_PULLUP);
  pinMode(buttonR, INPUT_PULLUP);
  pinMode(buttonK, INPUT_PULLUP);
  pinMode(buttonSkip, INPUT_PULLUP);
  pinMode(buttonLeft, INPUT_PULLUP);
  pinMode(buttonRight, INPUT_PULLUP);
  pinMode(buttonSelect, INPUT_PULLUP);
  
  // Initialize LEDs
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  
  turnOffAllLEDs();
  
  // ===== SD CARD INITIALIZATION (Hardware SPI) =====
  Serial.println(F("Initializing SD card..."));
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("INIT SD CARD..."));
  
  bool sdInitialized = false;
  
  // Method 1: Try with CS pin 4
  lcd.setCursor(0, 1);
  lcd.print(F("Trying CS=4..."));
  if (SD.begin(SD_CHIP_SELECT)) {
    sdInitialized = true;
    Serial.println(F("SD: CS=4 worked"));
  }
  
  // Method 2: Try with CS pin 10 (standard)
  if (!sdInitialized) {
    lcd.setCursor(0, 1);
    lcd.print(F("Trying CS=10..."));
    if (SD.begin(10)) {
      sdInitialized = true;
      Serial.println(F("SD: CS=10 worked"));
    }
  }
  
  // Method 3: Try with slower speed
  if (!sdInitialized) {
    lcd.setCursor(0, 1);
    lcd.print(F("Trying slow speed..."));
    // SD library doesn't support speed parameter directly
    // Just try begin again
    if (SD.begin(SD_CHIP_SELECT)) {
      sdInitialized = true;
      Serial.println(F("SD: Slow speed worked"));
    }
  }
  
  if (!sdInitialized) {
    Serial.println(F("SD card initialization failed!"));
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("SD CARD ERROR"));
    lcd.setCursor(0, 1);
    lcd.print(F("Check:"));
    lcd.setCursor(0, 2);
    lcd.print(F("1. 3.3V power"));
    lcd.setCursor(0, 3);
    lcd.print(F("2. Card format"));
    while (1); // Halt if SD card fails
  }
  
  Serial.println(F("SD card initialized successfully."));
  lcd.clear();
  
  // Scan for theme files
  scanThemes();
  
  randomSeed(analogRead(A5));
  
  Serial.println(F("=== SYSTEM STARTED ==="));
  
  displayPlayerMenu();
}

void loop() {
  if (gameState == 0) {
    handlePlayerMenu();
  } else if (gameState == 1) {
    handleThemeMenu();
  } else if (gameState == 2) {
    handleGame();
  } else if (gameState == 3) {
    handleWinnerScreen();
  } else if (gameState == 4) {
    handleScoresScreen();
  }
}

// ==================== SD CARD FUNCTIONS ====================
void scanThemes() {
  Serial.println(F("Scanning for theme files..."));
  
  totalThemes = 0;
  
  // Open root directory
  File root = SD.open("/");
  if (!root) {
    Serial.println(F("Failed to open root directory"));
    return;
  }
  
  while (true) {
    File entry = root.openNextFile();
    if (!entry) {
      // No more files
      break;
    }
    
    if (!entry.isDirectory()) {
      // Use name() instead of getName()
      char* filename = entry.name();
      
      // Check if file is a CSV file
      if (filename) {
        int len = strlen(filename);
        if (len >= 4) {
          // Extract extension
          char ext[5];
          strcpy(ext, filename + len - 4);
          
          // Convert extension to uppercase for comparison
          for (int i = 0; i < 4; i++) {
            ext[i] = toupper(ext[i]);
          }
          
          if (strcmp(ext, ".CSV") == 0) {
            if (totalThemes < 20) {
              // Remove .csv extension (4 characters)
              strncpy(themeNames[totalThemes], filename, len - 4);
              themeNames[totalThemes][len - 4] = '\0'; // Null terminate
              
              // Convert theme name to uppercase for display
              for (int i = 0; themeNames[totalThemes][i]; i++) {
                themeNames[totalThemes][i] = toupper(themeNames[totalThemes][i]);
              }
              
              Serial.print(F("Found theme: "));
              Serial.println(themeNames[totalThemes]);
              totalThemes++;
            }
          }
        }
      }
    }
    entry.close();
  }
  
  root.close();
  
  if (totalThemes == 0) {
    Serial.println(F("No theme files found!"));
  } else {
    Serial.print(F("Total themes found: "));
    Serial.println(totalThemes);
  }
}

bool loadTheme(byte themeIndex) {
  if (themeIndex >= totalThemes) {
    Serial.println(F("Invalid theme index"));
    return false;
  }
  
  // Create filename with .csv extension
  char filename[20];
  strcpy(filename, themeNames[themeIndex]);
  strcat(filename, ".CSV");
  
  Serial.print(F("Loading theme: "));
  Serial.println(filename);
  
  File themeFile = SD.open(filename);
  if (!themeFile) {
    Serial.println(F("Failed to open theme file"));
    return false;
  }
  
  // Read words from file
  totalWordsInTheme = 0;
  char lineBuffer[20];
  int lineIndex = 0;
  
  while (themeFile.available() && totalWordsInTheme < 20) {
    char c = themeFile.read();
    
    if (c == '\n' || c == '\r') {
      if (lineIndex > 0) {
        lineBuffer[lineIndex] = '\0';
        
        // Convert to uppercase
        for (int i = 0; lineBuffer[i]; i++) {
          lineBuffer[i] = toupper(lineBuffer[i]);
        }
        
        // Trim trailing spaces
        while (lineIndex > 0 && lineBuffer[lineIndex-1] == ' ') {
          lineBuffer[--lineIndex] = '\0';
        }
        
        if (lineIndex > 0) {  // Non-empty line
          strcpy(currentWords[totalWordsInTheme], lineBuffer);
          totalWordsInTheme++;
        }
        lineIndex = 0;
      }
    } else if (lineIndex < 19) {
      lineBuffer[lineIndex++] = c;
    }
  }
  
  // Check last line if file doesn't end with newline
  if (lineIndex > 0 && totalWordsInTheme < 20) {
    lineBuffer[lineIndex] = '\0';
    for (int i = 0; lineBuffer[i]; i++) {
      lineBuffer[i] = toupper(lineBuffer[i]);
    }
    strcpy(currentWords[totalWordsInTheme], lineBuffer);
    totalWordsInTheme++;
  }
  
  themeFile.close();
  
  Serial.print(F("Loaded "));
  Serial.print(totalWordsInTheme);
  Serial.println(F(" words"));
  
  if (totalWordsInTheme < 20) {
    Serial.println(F("WARNING: Theme has less than 20 words!"));
  }
  
  return true;
}

// ==================== HELPER FUNCTIONS ====================
void turnOffAllLEDs() {
  digitalWrite(led1, LOW);
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
  digitalWrite(led4, LOW);
}

byte countBits(unsigned long n) {
  byte count = 0;
  while (n) {
    count += n & 1;
    n >>= 1;
  }
  return count;
}

// ==================== PLAYER MENU ====================
void displayPlayerMenu() {
  gameState = 0;
  scoresScreenPage = 0;
  turnOffAllLEDs();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("SELECT PLAYERS"));
  lcd.setCursor(0, 1);
  lcd.print(F("1    2    3    4"));
  updatePlayerCursor();
}

void updatePlayerCursor() {
  lcd.setCursor(0, 2);
  lcd.print(F("                    "));
  lcd.setCursor(selectedPlayers * 5 - 5, 2);
  lcd.print(F("^"));
  lcd.setCursor(0, 3);
  lcd.print(F("<- -> NAV  SEL OK"));
}

void handlePlayerMenu() {
  if (digitalRead(buttonLeft) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    selectedPlayers = (selectedPlayers - 1);
    if (selectedPlayers < 1) selectedPlayers = 4;
    updatePlayerCursor();
    Serial.print(F("Players: "));
    Serial.println(selectedPlayers);
    while(digitalRead(buttonLeft) == LOW) delay(10);
  }
  
  if (digitalRead(buttonRight) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    selectedPlayers = (selectedPlayers % 4) + 1;
    updatePlayerCursor();
    Serial.print(F("Players: "));
    Serial.println(selectedPlayers);
    while(digitalRead(buttonRight) == LOW) delay(10);
  }
  
  if (digitalRead(buttonSelect) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    currentThemeIndex = 0;
    displayThemeMenu();
    while(digitalRead(buttonSelect) == LOW) delay(10);
  }
}

// ==================== THEME MENU ====================
void displayThemeMenu() {
  gameState = 1;
  turnOffAllLEDs();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("SELECT THEME"));
  
  if (totalThemes == 0) {
    lcd.setCursor(0, 1);
    lcd.print(F("NO THEMES FOUND"));
    lcd.setCursor(0, 2);
    lcd.print(F("Add CSV files to"));
    lcd.setCursor(0, 3);
    lcd.print(F("SD card root"));
    return;
  }
  
  // Display current theme name
  lcd.setCursor(0, 1);
  lcd.print(F("                    "));
  lcd.setCursor(0, 1);
  lcd.print(themeNames[currentThemeIndex]);
  
  // Show theme count
  lcd.setCursor(0, 2);
  lcd.print(F("< PREV    NEXT >"));
  lcd.setCursor(0, 3);
  lcd.print(F("SEL: Confirm"));
}

void handleThemeMenu() {
  if (totalThemes == 0) {
    // No themes available
    if (digitalRead(buttonSelect) == LOW && millis() - lastButtonPress > debounceDelay) {
      lastButtonPress = millis();
      displayPlayerMenu();
      while(digitalRead(buttonSelect) == LOW) delay(10);
    }
    return;
  }
  
  if (digitalRead(buttonLeft) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    currentThemeIndex = (currentThemeIndex - 1 + totalThemes) % totalThemes;
    lcd.setCursor(0, 1);
    lcd.print(F("                    "));
    lcd.setCursor(0, 1);
    lcd.print(themeNames[currentThemeIndex]);
    Serial.print(F("Theme: "));
    Serial.println(themeNames[currentThemeIndex]);
    while(digitalRead(buttonLeft) == LOW) delay(10);
  }
  
  if (digitalRead(buttonRight) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    currentThemeIndex = (currentThemeIndex + 1) % totalThemes;
    lcd.setCursor(0, 1);
    lcd.print(F("                    "));
    lcd.setCursor(0, 1);
    lcd.print(themeNames[currentThemeIndex]);
    Serial.print(F("Theme: "));
    Serial.println(themeNames[currentThemeIndex]);
    while(digitalRead(buttonRight) == LOW) delay(10);
  }
  
  if (digitalRead(buttonSelect) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    
    // Load selected theme
    if (loadTheme(currentThemeIndex)) {
      if (totalWordsInTheme >= 20) {
        // Reset all scores for new game
        for (byte player = 0; player < 4; player++) {
          for (byte mode = 0; mode < 4; mode++) {
            playerScores[player][mode] = 0;
          }
        }
        usedWords = 0;
        currentPlayer = 0;
        gameState = 2;
        displayNextWord();
      } else {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(F("INSUFFICIENT WORDS"));
        lcd.setCursor(0, 1);
        lcd.print(F("Need 20, found:"));
        lcd.setCursor(0, 2);
        lcd.print(totalWordsInTheme);
        lcd.setCursor(0, 3);
        lcd.print(F("Press SEL to go back"));
        
        // Wait for select to go back
        while(true) {
          if (digitalRead(buttonSelect) == LOW && millis() - lastButtonPress > debounceDelay) {
            lastButtonPress = millis();
            displayThemeMenu();
            while(digitalRead(buttonSelect) == LOW) delay(10);
            break;
          }
          delay(50);
        }
      }
    }
    
    while(digitalRead(buttonSelect) == LOW) delay(10);
  }
}

// ==================== GAME FUNCTIONS ====================
void displayNextWord() {
  lcd.clear();
  
  // Get random unused word
  byte wordIndex;
  int attempts = 0;
  do {
    wordIndex = random(totalWordsInTheme);
    attempts++;
    if (attempts > 100) {
      usedWords = 0; // Reset if we can't find unused word
      wordIndex = random(totalWordsInTheme);
      break;
    }
  } while ((usedWords >> wordIndex) & 1);
  
  usedWords |= (1UL << wordIndex);
  
  // Display on LCD
  lcd.setCursor(0, 0);
  lcd.print(F("PLAYER "));
  lcd.print(currentPlayer + 1);
  lcd.print(F(" OF "));
  lcd.print(selectedPlayers);
  
  lcd.setCursor(0, 1);
  lcd.print(F("WORD "));
  lcd.print(countBits(usedWords));
  lcd.print(F(" OF "));
  lcd.print(totalWordsInTheme);
  
  // Center the word
  int wordLength = strlen(currentWords[wordIndex]);
  int startPos = (20 - wordLength) / 2;
  lcd.setCursor(startPos, 2);
  lcd.print(currentWords[wordIndex]);
  
  // Clear the button prompt line - it will be filled in updateButtonPrompt()
  lcd.setCursor(0, 3);
  lcd.print(F("                    "));
  
  Serial.println(F("========================="));
  Serial.print(F("Player "));
  Serial.print(currentPlayer + 1);
  Serial.print(F(" turn | Word: "));
  Serial.print(currentWords[wordIndex]);
}

void updateButtonPrompt() {
  lcd.setCursor(0, 3);
  lcd.print(F("                    ")); // Clear the line
  lcd.setCursor(0, 3);
  
  if (currentLed == 0) {
    lcd.print(F("Answer by V mode"));
  } else if (currentLed == 1) {
    lcd.print(F("Answer by A mode"));
  } else if (currentLed == 2) {
    lcd.print(F("Answer by R mode"));
  } else {
    lcd.print(F("Answer by K mode"));
  }
  
  Serial.print(F(" | LED "));
  Serial.print(currentLed + 1);
  Serial.print(F(" - Answer by "));
  Serial.println(currentLed == 0 ? "V" : currentLed == 1 ? "A" : currentLed == 2 ? "R" : "K");
}

void handleGame() {
  // Light random LED at the start of each turn
  if (!ledLit) {
    currentLed = random(4);
    turnOffAllLEDs();
    switch(currentLed) {
      case 0: digitalWrite(led1, HIGH); break;
      case 1: digitalWrite(led2, HIGH); break;
      case 2: digitalWrite(led3, HIGH); break;
      case 3: digitalWrite(led4, HIGH); break;
    }
    ledLit = true;
    
    // Update the button prompt AFTER setting the LED
    updateButtonPrompt();
  }
  
  checkButton(buttonV, 0);
  checkButton(buttonA, 1);
  checkButton(buttonR, 2);
  checkButton(buttonK, 3);
  checkSkipButton();
}

void checkButton(int buttonPin, byte buttonType) {
  if (digitalRead(buttonPin) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    
    Serial.print(F("Button "));
    Serial.print(buttonType == 0 ? "V" : buttonType == 1 ? "A" : buttonType == 2 ? "R" : "K");
    Serial.println(F(" pressed"));
    
    // Check if this is the current player's turn
    if (buttonType == currentLed) {
      playerScores[currentPlayer][buttonType]++;
      Serial.print(F(">>> Player "));
      Serial.print(currentPlayer + 1);
      Serial.print(F(" scores in "));
      Serial.print(buttonType == 0 ? "V" : buttonType == 1 ? "A" : buttonType == 2 ? "R" : "K");
      Serial.print(F(" mode! "));
      Serial.print(F("Total in this mode: "));
      Serial.println(playerScores[currentPlayer][buttonType]);
    } else {
      Serial.println(F(">>> WRONG mode! No score."));
    }
    
    // Move to next player
    currentPlayer = (currentPlayer + 1) % selectedPlayers;
    
    if (countBits(usedWords) >= totalWordsInTheme || countBits(usedWords) >= 20) {
      gameState = 3;
      displayWinnerScreen();
    } else {
      // Reset LED flag for next word
      ledLit = false;
      turnOffAllLEDs(); // Turn off LEDs before next word
      displayNextWord();
    }
    
    while(digitalRead(buttonPin) == LOW) delay(10);
  }
}

void checkSkipButton() {
  if (digitalRead(buttonSkip) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    Serial.println(F("Skip pressed - no score"));
    
    currentPlayer = (currentPlayer + 1) % selectedPlayers;
    
    if (countBits(usedWords) >= totalWordsInTheme || countBits(usedWords) >= 20) {
      gameState = 3;
      displayWinnerScreen();
    } else {
      // Reset LED flag for next word
      ledLit = false;
      turnOffAllLEDs(); // Turn off LEDs before next word
      displayNextWord();
    }
    
    while(digitalRead(buttonSkip) == LOW) delay(10);
  }
}

// ==================== WINNER SCREEN ====================
void displayWinnerScreen() {
  turnOffAllLEDs();
  gameState = 3;
  scoresScreenPage = 0;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("    GAME OVER"));
  
  // Calculate total scores
  int totalScores[4] = {0, 0, 0, 0};
  for (byte player = 0; player < selectedPlayers; player++) {
    for (byte mode = 0; mode < 4; mode++) {
      totalScores[player] += playerScores[player][mode];
    }
  }
  
  // Find winner
  int maxScore = 0;
  byte winner = 0;
  for (byte player = 0; player < selectedPlayers; player++) {
    if (totalScores[player] > maxScore) {
      maxScore = totalScores[player];
      winner = player;
    }
  }
  
  // Check for ties
  byte tieCount = 0;
  for (byte player = 0; player < selectedPlayers; player++) {
    if (totalScores[player] == maxScore) {
      tieCount++;
    }
  }
  
  // Display winner/tie
  lcd.setCursor(0, 1);
  if (tieCount > 1) {
    lcd.print(F("    TIE GAME!"));
    lcd.setCursor(0, 2);
    lcd.print(F("Multiple winners"));
  } else {
    lcd.print(F("   WINNER IS"));
    lcd.setCursor(0, 2);
    lcd.print(F("    PLAYER "));
    lcd.print(winner + 1);
  }
  
  lcd.setCursor(0, 3);
  lcd.print(F("Press SEL for scores"));
}

void handleWinnerScreen() {
  if (digitalRead(buttonSelect) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    gameState = 4;
    displayScoresScreen();
    while(digitalRead(buttonSelect) == LOW) delay(10);
  }
}

// ==================== SCORES SCREEN ====================
void displayScoresScreen() {
  lcd.clear();
  lcd.setCursor(0, 0);
  
  if (selectedPlayers > 2) {
    lcd.print(F("SCORES "));
    if (selectedPlayers == 3) {
      lcd.print(F("(1/1)"));
    } else if (selectedPlayers == 4) {
      lcd.print(F("("));
      lcd.print(scoresScreenPage + 1);
      lcd.print(F("/2)"));
    }
  } else {
    lcd.print(F("INDIVIDUAL SCORES"));
  }
  
  byte startPlayer = scoresScreenPage * 2;
  byte playersOnThisScreen = min(2, selectedPlayers - startPlayer);
  
  for (byte i = 0; i < playersOnThisScreen; i++) {
    byte player = startPlayer + i;
    lcd.setCursor(0, 1 + i);
    
    lcd.print(F("P"));
    lcd.print(player + 1);
    lcd.print(F(": V"));
    lcd.print(playerScores[player][0]);
    lcd.print(F(" A"));
    lcd.print(playerScores[player][1]);
    lcd.print(F(" R"));
    lcd.print(playerScores[player][2]);
    lcd.print(F(" K"));
    lcd.print(playerScores[player][3]);
    
    int total = playerScores[player][0] + playerScores[player][1] + 
                playerScores[player][2] + playerScores[player][3];
    lcd.print(F(" T:"));
    lcd.print(total);
  }
  
  lcd.setCursor(0, 3);
  
  if (selectedPlayers <= 2) {
    lcd.print(F("SEL:Menu"));
  } else if (selectedPlayers == 3) {
    lcd.print(F("SEL:Menu"));
  } else if (selectedPlayers == 4) {
    if (scoresScreenPage == 0) {
      lcd.print(F("<->:More  SEL:Menu"));
    } else {
      lcd.print(F("<->:Back  SEL:Menu"));
    }
  }
}

void handleScoresScreen() {
  if (digitalRead(buttonLeft) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    
    if (selectedPlayers == 4) {
      scoresScreenPage = (scoresScreenPage - 1 + 2) % 2;
      displayScoresScreen();
    }
    
    while(digitalRead(buttonLeft) == LOW) delay(10);
  }
  
  if (digitalRead(buttonRight) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    
    if (selectedPlayers == 4) {
      scoresScreenPage = (scoresScreenPage + 1) % 2;
      displayScoresScreen();
    }
    
    while(digitalRead(buttonRight) == LOW) delay(10);
  }
  
  if (digitalRead(buttonSelect) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    scoresScreenPage = 0;
    displayPlayerMenu();
    while(digitalRead(buttonSelect) == LOW) delay(10);
  }
}
