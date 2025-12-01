#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// LCD configuration for I2C - Changed to 20x4
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Button pin configuration
const int buttonV = 6;
const int buttonA = 7;
const int buttonR = 8;
const int buttonK = 9;
const int buttonSkip = 10;

// Navigation buttons
const int buttonLeft = 11;
const int buttonRight = 12;
const int buttonSelect = 13;

// LED pin configuration
const int led1 = A0;
const int led2 = A1;
const int led3 = A2;
const int led4 = 2;

// Game state
byte gameState = 0; // 0=player menu, 1=theme menu, 2=playing, 3=winner screen, 4=scores screen
byte selectedPlayers = 1;
byte currentTheme = 0;
unsigned long usedWords = 0;
byte currentLed = 0;
byte currentPlayer = 0;

// Scores screen navigation
byte scoresScreenPage = 0; // 0 = first page, 1 = second page (for 4 players)

// Individual mode scores for each player: playerScores[player][mode]
int playerScores[4][4] = {
  {0, 0, 0, 0}, // Player 1: V, A, R, K
  {0, 0, 0, 0}, // Player 2: V, A, R, K
  {0, 0, 0, 0}, // Player 3: V, A, R, K
  {0, 0, 0, 0}  // Player 4: V, A, R, K
};

// Button state tracking
unsigned long lastButtonPress = 0;
const unsigned long debounceDelay = 300;

// Store words in PROGMEM
const char themes[] PROGMEM = "SPORTS\0LITERATURE\0EDUCATION\0ENTERTAINMENT\0";

const char sportsWords[] PROGMEM = 
  "FOOTBALL\0BASKETBALL\0TENNIS\0CRICKET\0SWIMMING\0"
  "ATHLETICS\0VOLLEYBALL\0HOCKEY\0BASEBALL\0GOLF\0"
  "BOXING\0CYCLING\0SURFING\0SKIING\0ARCHERY\0"
  "FENCING\0JUDO\0WRESTLING\0ROWING\0SAILING\0";

const char literatureWords[] PROGMEM = 
  "NOVEL\0POETRY\0DRAMA\0FICTION\0BIOGRAPHY\0"
  "MYSTERY\0ROMANCE\0FANTASY\0SCIENCE\0HISTORY\0"
  "PHILOSOPHY\0ESSAY\0COMEDY\0TRAGEDY\0SATIRE\0"
  "METAPHOR\0ALLEGORY\0SYMBOLISM\0PLOT\0CHARACTER\0";

const char educationWords[] PROGMEM = 
  "MATH\0SCIENCE\0HISTORY\0GEOGRAPHY\0LANGUAGE\0"
  "ART\0MUSIC\0PHYSICS\0CHEMISTRY\0BIOLOGY\0"
  "ALGEBRA\0GEOMETRY\0GRAMMAR\0VOCAB\0RESEARCH\0"
  "EXPERIMENT\0THEORY\0PRACTICAL\0ANALYSIS\0SYNTHESIS\0";

const char entertainmentWords[] PROGMEM = 
  "MOVIES\0MUSIC\0THEATER\0DANCE\0TV\0"
  "GAMING\0COMEDY\0CONCERT\0FESTIVAL\0CARNIVAL\0"
  "CARTOON\0ANIMATION\0BROADWAY\0OPERA\0BALLET\0"
  "CINEMA\0DRAMA\0SERIES\0DOC\0REALITY\0";

// Buffer for reading from PROGMEM
char wordBuffer[20];
char themeBuffer[20];

void setup() {
  Serial.begin(9600);
  
  lcd.init();
  lcd.backlight();
  lcd.clear();
  
  pinMode(buttonV, INPUT_PULLUP);
  pinMode(buttonA, INPUT_PULLUP);
  pinMode(buttonR, INPUT_PULLUP);
  pinMode(buttonK, INPUT_PULLUP);
  pinMode(buttonSkip, INPUT_PULLUP);
  pinMode(buttonLeft, INPUT_PULLUP);
  pinMode(buttonRight, INPUT_PULLUP);
  pinMode(buttonSelect, INPUT_PULLUP);
  
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);
  pinMode(led4, OUTPUT);
  
  turnOffAllLEDs();
  
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

// ==================== HELPER FUNCTIONS ====================
void getStringFromPROGMEM(const char* source, byte index, char* buffer) {
  const char* ptr = source;
  for (byte i = 0; i < index; i++) {
    while (pgm_read_byte(ptr) != 0) ptr++;
    ptr++;
  }
  
  byte i = 0;
  char c;
  while ((c = pgm_read_byte(ptr)) != 0 && i < 19) {
    buffer[i++] = c;
    ptr++;
  }
  buffer[i] = '\0';
}

void getWord(byte theme, byte index, char* buffer) {
  const char* wordList;
  switch(theme) {
    case 0: wordList = sportsWords; break;
    case 1: wordList = literatureWords; break;
    case 2: wordList = educationWords; break;
    case 3: wordList = entertainmentWords; break;
  }
  getStringFromPROGMEM(wordList, index, buffer);
}

void getThemeName(byte index, char* buffer) {
  getStringFromPROGMEM(themes, index, buffer);
}

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
  scoresScreenPage = 0; // Reset scores screen page
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
    currentTheme = 0;
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
  getThemeName(currentTheme, themeBuffer);
  lcd.setCursor(0, 1);
  lcd.print(themeBuffer);
  lcd.setCursor(0, 2);
  lcd.print(F("< PREV    NEXT >"));
  lcd.setCursor(0, 3);
  lcd.print(F("SEL: Confirm"));
}

void handleThemeMenu() {
  if (digitalRead(buttonLeft) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    currentTheme = (currentTheme - 1 + 4) % 4;
    getThemeName(currentTheme, themeBuffer);
    lcd.setCursor(0, 1);
    lcd.print(F("                    "));
    lcd.setCursor(0, 1);
    lcd.print(themeBuffer);
    Serial.print(F("Theme: "));
    Serial.println(themeBuffer);
    while(digitalRead(buttonLeft) == LOW) delay(10);
  }
  
  if (digitalRead(buttonRight) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    currentTheme = (currentTheme + 1) % 4;
    getThemeName(currentTheme, themeBuffer);
    lcd.setCursor(0, 1);
    lcd.print(F("                    "));
    lcd.setCursor(0, 1);
    lcd.print(themeBuffer);
    Serial.print(F("Theme: "));
    Serial.println(themeBuffer);
    while(digitalRead(buttonRight) == LOW) delay(10);
  }
  
  if (digitalRead(buttonSelect) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
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
    while(digitalRead(buttonSelect) == LOW) delay(10);
  }
}

// ==================== GAME FUNCTIONS ====================
void displayNextWord() {
  lcd.clear();
  
  // Get random unused word (0-19)
  byte wordIndex;
  int attempts = 0;
  do {
    wordIndex = random(20);
    attempts++;
    if (attempts > 100) {
      usedWords = 0;
      wordIndex = random(20);
      break;
    }
  } while ((usedWords >> wordIndex) & 1);
  
  usedWords |= (1UL << wordIndex);
  
  // Get word from PROGMEM
  getWord(currentTheme, wordIndex, wordBuffer);
  
  // Light random LED (0-3)
  currentLed = random(4);
  turnOffAllLEDs();
  switch(currentLed) {
    case 0: digitalWrite(led1, HIGH); break;
    case 1: digitalWrite(led2, HIGH); break;
    case 2: digitalWrite(led3, HIGH); break;
    case 3: digitalWrite(led4, HIGH); break;
  }
  
  // Display on LCD
  lcd.setCursor(0, 0);
  lcd.print(F("PLAYER "));
  lcd.print(currentPlayer + 1);
  lcd.print(F(" OF "));
  lcd.print(selectedPlayers);
  
  lcd.setCursor(0, 1);
  lcd.print(F("WORD "));
  lcd.print(countBits(usedWords));
  lcd.print(F(" OF 20"));
  
  // Center the word
  int startPos = (20 - strlen(wordBuffer)) / 2;
  lcd.setCursor(startPos, 2);
  lcd.print(wordBuffer);
  
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
  
  Serial.println(F("========================="));
  Serial.print(F("Player "));
  Serial.print(currentPlayer + 1);
  Serial.print(F(" turn | Word: "));
  Serial.print(wordBuffer);
  Serial.print(F(" | LED "));
  Serial.print(currentLed + 1);
  Serial.print(F(" - Answer by "));
  Serial.println(currentLed == 0 ? "V" : currentLed == 1 ? "A" : currentLed == 2 ? "R" : "K");
}

void handleGame() {
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
    // Only current player can score on their turn
    if (buttonType == currentLed) {
      // Correct button pressed AND correct mode
      // Increment the score for current player in the current mode
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
    
    // Display current player's scores for debugging
    Serial.print(F("Player "));
    Serial.print(currentPlayer + 1);
    Serial.print(F(" scores - V:"));
    Serial.print(playerScores[currentPlayer][0]);
    Serial.print(F(" A:"));
    Serial.print(playerScores[currentPlayer][1]);
    Serial.print(F(" R:"));
    Serial.print(playerScores[currentPlayer][2]);
    Serial.print(F(" K:"));
    Serial.println(playerScores[currentPlayer][3]);
    
    // Move to next player
    currentPlayer = (currentPlayer + 1) % selectedPlayers;
    
    if (countBits(usedWords) >= 20) {
      gameState = 3;
      displayWinnerScreen();
    } else {
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
    
    if (countBits(usedWords) >= 20) {
      gameState = 3;
      displayWinnerScreen();
    } else {
      displayNextWord();
    }
    
    while(digitalRead(buttonSkip) == LOW) delay(10);
  }
}

// ==================== WINNER SCREEN ====================
void displayWinnerScreen() {
  turnOffAllLEDs();
  gameState = 3;
  scoresScreenPage = 0; // Reset to first page
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(F("    GAME OVER"));
  
  // Calculate total scores for each player
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
  
  Serial.println(F("=== GAME OVER ==="));
  if (tieCount > 1) {
    Serial.println(F("TIE GAME!"));
  } else {
    Serial.print(F("WINNER: Player "));
    Serial.println(winner + 1);
  }
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
  
  // Show page indicator if multiple pages
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
  
  // Display 2 players per screen (rows 1 and 2)
  byte startPlayer = scoresScreenPage * 2;
  byte playersOnThisScreen = min(2, selectedPlayers - startPlayer);
  
  for (byte i = 0; i < playersOnThisScreen; i++) {
    byte player = startPlayer + i;
    lcd.setCursor(0, 1 + i);
    
    // Player number
    lcd.print(F("P"));
    lcd.print(player + 1);
    lcd.print(F(": "));
    
    // Mode scores
    lcd.print(F("V"));
    lcd.print(playerScores[player][0]);
    lcd.print(F(" A"));
    lcd.print(playerScores[player][1]);
    lcd.print(F(" R"));
    lcd.print(playerScores[player][2]);
    lcd.print(F(" K"));
    lcd.print(playerScores[player][3]);
    
    // Total score
    int total = playerScores[player][0] + playerScores[player][1] + 
                playerScores[player][2] + playerScores[player][3];
    lcd.print(F(" T:"));
    lcd.print(total);
  }
  
  // Navigation instructions on row 4
  lcd.setCursor(0, 3);
  
  if (selectedPlayers <= 2) {
    // 1-2 players: Only one screen
    lcd.print(F("SEL:Menu"));
  } else if (selectedPlayers == 3) {
    // 3 players: Only one screen (shows P1-P2, row 3 empty)
    lcd.print(F("SEL:Menu"));
  } else if (selectedPlayers == 4) {
    // 4 players: Two screens
    if (scoresScreenPage == 0) {
      // First screen: Show P1-P2
      lcd.print(F("<->:More  SEL:Menu"));
    } else {
      // Second screen: Show P3-P4  
      lcd.print(F("<->:Back  SEL:Menu"));
    }
  }
}

void handleScoresScreen() {
  // Check for navigation buttons
  if (digitalRead(buttonLeft) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    
    if (selectedPlayers == 4) {
      // Cycle backward through pages
      scoresScreenPage = (scoresScreenPage - 1 + 2) % 2;
      displayScoresScreen();
    }
    
    while(digitalRead(buttonLeft) == LOW) delay(10);
  }
  
  if (digitalRead(buttonRight) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    
    if (selectedPlayers == 4) {
      // Cycle forward through pages
      scoresScreenPage = (scoresScreenPage + 1) % 2;
      displayScoresScreen();
    }
    
    while(digitalRead(buttonRight) == LOW) delay(10);
  }
  
  // Check for return to menu
  if (digitalRead(buttonSelect) == LOW && millis() - lastButtonPress > debounceDelay) {
    lastButtonPress = millis();
    scoresScreenPage = 0; // Reset page
    displayPlayerMenu();
    while(digitalRead(buttonSelect) == LOW) delay(10);
  }
}
