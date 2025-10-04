#include "common.h"
#include "common/common.h"
#include "lidar.h"

#define SONAR_FRONT_MIN_TO_TRIGGER 48

#define PIN_PIR1 10
#define PIN_PIR2 9

#define PIN_TRIGGER_PROX 19
#define PIN_TRIGGER_PIR   18

// From motor boards
#define PIN_IN_BLOOM1 20
#define PIN_IN_BLOOM2 21

lidar_t lidar;
bool setup_0_done = false;
debounce_n_t debounce;

void setup1() {
  while(!setup_0_done) { delay(1); }

  Serial.println("Starting...");
}

void setup() {
  Serial.begin(9600);

  pinMode(PIN_PIR1, INPUT_PULLDOWN);
  pinMode(PIN_PIR2, INPUT_PULLDOWN);

  pinMode(PIN_TRIGGER_PROX, OUTPUT);
  pinMode(PIN_TRIGGER_PIR, OUTPUT);
  digitalWrite(PIN_TRIGGER_PROX, LOW);
  digitalWrite(PIN_TRIGGER_PIR, LOW);

  pinMode(PIN_IN_BLOOM1, INPUT_PULLDOWN);
  pinMode(PIN_IN_BLOOM2, INPUT_PULLDOWN);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  
  wait_serial();
  i2c_scan();
  lidar.init();

  setup_0_done = true;

  Serial.print("Pico Chip ID: "); Serial.println(rp2040.getChipID());
}

void benchmark() {
  static uint32_t iterations = 0;
  static float avg = 0;
  static uint32_t last = 0;
  static uint32_t last_output = millis();

  uint32_t now = millis();
  uint32_t diff = now - last;
  last = now;

  if(!iterations)
    avg = diff;
  else
    // Poor-mans moving average
    avg = (avg * 4 + diff)/5;

  // NOTE: This will skew the measurement for the next iteration
  // Need extra work to keep accurate, but that's a pretty low priority
  if(now - last_output < 2000)
    return;

  Serial.printf("Benchmark: %f ms\n", avg);
  last_output = now;
}

bool do_pir(int in_pin, bool log) {
  bool state = digitalRead(in_pin);

  if (log)
    Serial.printf("PIR %d: %s\n", in_pin, state ? "motion!" : "no motion");

  return state;
}

void loop() {
  blink();

  // XXX This must be on the same core as the sonar since sonar'ing
  // holds the address
  // mux.next();

  // benchmark();

  //Serial.printf("Ax: %ld %ld %ld\n", analogRead(A0), analogRead(A1), analogRead(A2));
  //delay(100);

  bool log = false;
  static uint32_t last_log = 0;
  uint32_t now = millis();
  if(now - last_log > 250) {
    last_log = now;
    log = true;
  }

  if(!lidar.next()) {
    if(log) {
        Serial.println("Lidar read failed");
        i2c_scan();
        lidar.init();
    }
    delay(500);
    return;
  }
  
  if(log) {
    Serial.printf("Lidar: Dist: %din Str: %d Temp: %dC\n", 
      lidar.distance, lidar.strength, lidar.temp);
  }

  // Facing seat. Trigger initial bloom
  uint16_t s1 = lidar.distance; // do_sonar(PIN_SONAR1_OUT, PIN_SONAR1_IN, log);

  if(s1 && debounce.update(s1, SONAR_FRONT_MIN_TO_TRIGGER)) {
    Serial.printf("Prox triggered - %d\n", s1);
    digitalWrite(PIN_TRIGGER_PROX, HIGH);
  }
  else {
    digitalWrite(PIN_TRIGGER_PROX, LOW);
  }

  bool p1 = do_pir(PIN_PIR1, log);
  bool p2 = do_pir(PIN_PIR2, log);

  // Let the motor controllers decide
  if(p1 || p2) {
    digitalWrite(PIN_TRIGGER_PIR, HIGH);
  }
  else {
    digitalWrite(PIN_TRIGGER_PIR, LOW);
  }
}