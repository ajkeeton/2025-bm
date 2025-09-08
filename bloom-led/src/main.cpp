#include "common/common.h"
#include "leds.h"

#define SONAR_FRONT_MIN_TO_TRIGGER 30
// #define SONAR_BLOOM_MIN_TO_TRIGGER 25

#define PIN_SONAR1_IN 14
#define PIN_SONAR1_OUT 13

#define PIN_SONAR2_IN 12
#define PIN_SONAR2_OUT 11

#define PIN_PIR1 10
#define PIN_PIR2 9

#define PIN_TRIGGER_SONAR 18
#define PIN_TRIGGER_PIR   19

// From motor boards
#define PIN_IN_BLOOM1 20
#define PIN_IN_BLOOM2 21

leds_t leds;
bool setup_0_done = false;
debounce_n_t debounce;

void setup1() {
  wait_serial();
  while(!setup_0_done) { delay(1); }

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("Starting...");
}

void setup() {
  Serial.begin(9600);
  leds.init();

  pinMode(PIN_SONAR1_OUT, OUTPUT); 
  digitalWrite(PIN_SONAR1_OUT, LOW);
  pinMode(PIN_SONAR2_OUT, OUTPUT);
  digitalWrite(PIN_SONAR2_OUT, LOW);

  pinMode(PIN_SONAR1_IN, INPUT);
  pinMode(PIN_SONAR2_IN, INPUT);
  pinMode(PIN_PIR1, INPUT_PULLDOWN);
  pinMode(PIN_PIR2, INPUT_PULLDOWN);

  pinMode(PIN_TRIGGER_SONAR, OUTPUT);
  pinMode(PIN_TRIGGER_PIR, OUTPUT);
  digitalWrite(PIN_TRIGGER_SONAR, LOW);
  digitalWrite(PIN_TRIGGER_PIR, LOW);

  pinMode(PIN_IN_BLOOM1, INPUT_PULLDOWN);
  pinMode(PIN_IN_BLOOM2, INPUT_PULLDOWN);

  wait_serial();

  setup_0_done = true;

  Serial.print("Chip ID: "); Serial.println(rp2040.getChipID());
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

long ms_to_inch(long mu) {
  // According to Parallax's datasheet for the PING))), there are 73.746
  // microseconds per inch (i.e. sound travels at 1130 feet per second).
  // This gives the distance travelled by the ping, outbound and return,
  // so we divide by 2 to get the distance of the obstacle.
  // See: https://www.parallax.com/package/ping-ultrasonic-distance-sensor-downloads/
  return mu / 74 / 2; // mu / 74 / 2;
}

uint16_t do_sonar(int out_pin, int in_pin, bool log) {
  // The ping is triggered by a high pulse of 2 or more microseconds.
  // Give a short low pulse beforehand to ensure a clean low pulse:
  digitalWrite(out_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(out_pin, HIGH);
  delayMicroseconds(5);
  digitalWrite(out_pin, LOW);

  long duration = pulseIn(in_pin, HIGH, 25000); // timeout after 25ms (approx 14ft)

  // convert the time into a distance
  long inches = ms_to_inch(duration);

  if(log)
    Serial.printf("%d: Dur: %ld, To inch: %ldin\n", out_pin, duration, inches);

  return inches;
}

bool do_pir(int in_pin, bool log) {
  bool state = digitalRead(in_pin);

  if (log)
    Serial.printf("PIR %d: %s\n", in_pin, state ? "motion!" : "no motion");

  return state;
}

void loop1() {
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
  if(now - last_log > 750) {
    last_log = now;
    log = true;
  }

  // Facing seat. Trigger initial bloom
  uint16_t s1 = do_sonar(PIN_SONAR1_OUT, PIN_SONAR1_IN, log);

  if(s1 && debounce.update(s1, SONAR_FRONT_MIN_TO_TRIGGER)) {
    Serial.printf("sonar triggered - %d\n", s1);
    digitalWrite(PIN_TRIGGER_SONAR, HIGH);
    leds.handle_sonar();
  }
  else {
    // Serial.printf("sonar not triggered: %d\n", s1);
    digitalWrite(PIN_TRIGGER_SONAR, LOW);
  }

  // Only do this when the top has opened
  if(digitalRead(PIN_IN_BLOOM1) || digitalRead(PIN_IN_BLOOM2))
    if(log) Serial.printf("Currently in bloom: %d %d\n", 
        digitalRead(PIN_IN_BLOOM1), digitalRead(PIN_IN_BLOOM2));

    #if 0
    // LED changes, second bloom
    uint16_t s2 = do_sonar(PIN_SONAR2_OUT, PIN_SONAR2_IN, log);
    uint32_t s2pct = 0;

    s2pct = map(s2, 0, SONAR_BLOOM_MIN_TO_TRIGGER, 100, 0);
    s2pct = constrain(s2pct, 0, 100);
    leds.trigger(s2pct);

    if(log) 
      Serial.printf("leds_t::trigger - %d% -> %.2f\n", s2pct, leds.trigger_pct);

    //if(s2pct > 40) 
    //    digitalWrite(PIN_TRIGGER_BLOOM2, HIGH);
    //else
    //    digitalWrite(PIN_TRIGGER_BLOOM2, LOW);
  } 
  else {
    leds.trigger(0);
  }
  #endif

  bool p1 = do_pir(PIN_PIR1, log);
  bool p2 = do_pir(PIN_PIR2, log);

  // Let the motor controllers decide
  if(p1 || p2) {
    digitalWrite(PIN_TRIGGER_PIR, HIGH);

  }
  else {
    digitalWrite(PIN_TRIGGER_PIR, LOW);
  }

  // PIRs trigger LED change
  if(p1 || p2) 
    leds.handle_pir();
}
 
void loop() {
  leds.background_update();
  leds.step();
}
