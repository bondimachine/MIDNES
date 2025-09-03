#define PIN_DATA 0
#define PIN_LATCH 1
#define PIN_CLOCK 2

void setup() {
  
  Serial.begin(115200);

  Serial.println("NES MIDI adapter reader");

  pinMode(PIN_CLOCK, OUTPUT);
  pinMode(PIN_LATCH, OUTPUT);
  pinMode(PIN_DATA, INPUT);
}

void loop() {

  uint8_t buttons;

  digitalWrite(PIN_CLOCK, 1);
  digitalWrite(PIN_LATCH, 1);
  sleep_us(12);
  digitalWrite(PIN_LATCH, 0);

  sleep_us(6);

  buttons = digitalRead(PIN_DATA) == LOW; // A

  digitalWrite(PIN_CLOCK, 0);
  sleep_us(6);
  
  for (int i = 1; i <8; i++) {
    digitalWrite(PIN_CLOCK, 1);
    sleep_us(6);
    buttons |= ((digitalRead(PIN_DATA) == LOW) << i); 
    digitalWrite(PIN_CLOCK, 0);
    sleep_us(6);
  }

  static uint8_t count = 0;
  if (buttons != 0 || count != 0) {
    Serial.printf("%02x ", buttons);
    count++;
    if (count == 3) {
        Serial.println();
        count = 0;
    }
  }
}