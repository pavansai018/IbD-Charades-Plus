void setup() {
  Serial.begin(9600);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);
  pinMode(8, INPUT_PULLUP);
  pinMode(9, INPUT_PULLUP);
  pinMode(10, INPUT_PULLUP);
  Serial.println("Testing button wiring...");
}

void loop() {
  Serial.print("Buttons: ");
  Serial.print(digitalRead(6));
  Serial.print(digitalRead(7));
  Serial.print(digitalRead(8));
  Serial.print(digitalRead(9));
  Serial.print(digitalRead(10));
  Serial.println(" - Should be: 11111 (all HIGH)");
  delay(1000);
}
