void setup() {
  Serial.begin(9600);
  pinMode(A4, INPUT);
  pinMode(A5, INPUT);
}

void loop() {
  Serial.print("A4: "); Serial.println(analogRead(A4));
  Serial.print("A5: "); Serial.println(analogRead(A5));
  delay(1000);
}
