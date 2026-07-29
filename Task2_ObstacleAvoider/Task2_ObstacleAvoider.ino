/*
 * ST-2026 - Electrical Task 2  |  Sketch 2 of 2: servo + ultrasonic avoider
 *
 * Smart Methods (ST 2026) summer training.
 *
 *   The four DC motors of sketch 1, plus one servo motor carrying an
 *   ultrasonic sensor. The robot drives forward; when the sensor sees an
 *   obstacle 10 cm ahead the motors STOP and the direction changes: it backs
 *   off, the servo sweeps the sensor right and left to compare the free space
 *   on each side, and the robot turns toward the roomier one.
 *
 * Both sketches run on ONE circuit -- the same wiring, nothing to move
 * between them. Sketch 1 (Task1_FourMotorSequence) simply leaves the servo
 * and the sensor idle; this one uses everything.
 *
 * Wiring (Arduino Uno R3) -- full pin-by-pin table in docs/WIRING.md
 *
 *   L293D U1 (front)              L293D U2 (rear)
 *     pin 1  1,2EN -> D3  (PWM)     pin 1  1,2EN -> D6  (PWM)
 *     pin 2  1A    -> D2            pin 2  1A    -> D12
 *     pin 7  2A    -> D4            pin 7  2A    -> D13
 *     pin 9  3,4EN -> D5  (PWM)     pin 9  3,4EN -> D11 (PWM)
 *     pin 10 3A    -> D7            pin 10 3A    -> A0
 *     pin 15 4A    -> D8            pin 15 4A    -> A1
 *
 *   Both chips: pin 16 VCC1 -> 5V, pin 8 VCC2 -> 5V,
 *               pins 4/5/12/13 GND -> GND
 *   Motors:     U1 pins 3+6 -> front-left,  U1 pins 11+14 -> front-right
 *               U2 pins 3+6 -> rear-left,   U2 pins 11+14 -> rear-right
 *
 *   Servo (SG90)          Ultrasonic PING))) (3-pin, Tinkercad default)
 *     orange signal -> D10   SIG -> A2
 *     red    +      -> 5V    5V  -> 5V
 *     brown  -      -> GND   GND -> GND
 *
 *   A servo cannot be driven through an L293D -- it has its own control
 *   electronics and needs a PWM signal on one wire. That is not a workaround:
 *   the real L293D shield's servo headers are hard-wired straight to Arduino
 *   pins 10 and 9 and bypass the L293D chips entirely, so D10 here IS the
 *   shield's SERVO_1 pin.
 *
 *   D10 also works out because the Servo library takes over Timer1, which
 *   disables analogWrite() on pins 9 and 10 -- the two pins the four PWM
 *   enables (3, 5, 6, 11) deliberately avoid. Nothing is lost.
 *
 *   Using the 4-pin HC-SR04 instead? Set ULTRASONIC_4_PIN to 1 below and wire
 *   TRIG -> A2, ECHO -> A3.
 */

#include <Servo.h>

#define ULTRASONIC_4_PIN 0   // 0 = 3-pin PING))), 1 = 4-pin HC-SR04

// --- Motor identity ---------------------------------------------------------

const uint8_t FRONT_LEFT  = 0;
const uint8_t FRONT_RIGHT = 1;
const uint8_t REAR_LEFT   = 2;
const uint8_t REAR_RIGHT  = 3;
const uint8_t MOTOR_COUNT = 4;

// --- Pin map (identical to sketch 1) ----------------------------------------

const uint8_t MOTOR_EN[MOTOR_COUNT] = { 3,  5,  6, 11};  // L293D 1,2EN / 3,4EN
const uint8_t MOTOR_A [MOTOR_COUNT] = { 2,  7, 12, A0};  // L293D 1A / 3A
const uint8_t MOTOR_B [MOTOR_COUNT] = { 4,  8, 13, A1};  // L293D 2A / 4A

const bool MOTOR_FLIP[MOTOR_COUNT] = {false, false, false, false};

const uint8_t SERVO_PIN = 10;   // same pin as the L293D shield's SERVO_1 header
const uint8_t TRIG_PIN  = A2;   // PING))) SIG, or HC-SR04 TRIG
#if ULTRASONIC_4_PIN
const uint8_t ECHO_PIN  = A3;
#endif

// --- Behaviour --------------------------------------------------------------

const int STOP_DISTANCE_CM = 10;   // the task: react at 10 cm

const uint8_t DRIVE_SPEED = 180;
const uint8_t TURN_SPEED  = 160;

const int SCAN_RIGHT_DEG  =  30;   // servo angles for the look-around
const int SCAN_CENTER_DEG =  90;   // 90 deg = sensor pointing straight ahead
const int SCAN_LEFT_DEG   = 150;

const unsigned long SERVO_SETTLE_MS =  400;  // let the horn arrive before pinging
const unsigned long BACK_OFF_MS     =  600;  // reverse away from the obstacle
const unsigned long TURN_MS         =  700;  // how far to swing toward free space
const unsigned long PAUSE_MS        =  250;  // the visible "stop" between moves
const unsigned long PING_PERIOD_MS  =   60;  // how often to measure while driving

// Echo timeout: 30 ms of flight is about 5 m there and back, well past the
// PING)))'s range, so a silent return means "nothing in front of us".
const unsigned long ECHO_TIMEOUT_US = 30000UL;
const long          NO_ECHO         = -1;
const long          FAR_AWAY_CM     = 999;   // stand-in for NO_ECHO when comparing

Servo scanner;

// --- Low-level motor control (identical to sketch 1) ------------------------

// speed is signed: > 0 forward, < 0 reverse, 0 = stop.
void setMotor(uint8_t motor, int speed) {
  if (MOTOR_FLIP[motor]) {
    speed = -speed;
  }

  bool forwardDir = (speed > 0);
  uint8_t duty = (uint8_t) constrain(abs(speed), 0, 255);

  // A high, B low = one direction; A low, B high = the other; both low = stop.
  digitalWrite(MOTOR_A[motor], (duty > 0 &&  forwardDir) ? HIGH : LOW);
  digitalWrite(MOTOR_B[motor], (duty > 0 && !forwardDir) ? HIGH : LOW);
  analogWrite (MOTOR_EN[motor], duty);
}

void drive(int leftSpeed, int rightSpeed) {
  setMotor(FRONT_LEFT,  leftSpeed);
  setMotor(REAR_LEFT,   leftSpeed);
  setMotor(FRONT_RIGHT, rightSpeed);
  setMotor(REAR_RIGHT,  rightSpeed);
}

void forward()   { drive( DRIVE_SPEED,  DRIVE_SPEED); }
void backward()  { drive(-DRIVE_SPEED, -DRIVE_SPEED); }
void turnRight() { drive( TURN_SPEED,  -TURN_SPEED ); }  // tank turn, in place
void turnLeft()  { drive(-TURN_SPEED,   TURN_SPEED ); }
void stopAll()   { drive(0, 0); }

// --- Ultrasonic -------------------------------------------------------------

// Returns the distance in cm, or NO_ECHO when nothing answers in range.
//
// Sound covers 1 cm in about 29 us, and the pulse makes the trip twice, so
// centimetres = microseconds / 58.
long readDistanceCm() {
#if ULTRASONIC_4_PIN
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
#else
  // The 3-pin PING))) shares one wire: pulse it as an output to fire the
  // chirp, then flip the same pin to an input to time the echo.
  pinMode(TRIG_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, LOW);

  pinMode(TRIG_PIN, INPUT);
  unsigned long duration = pulseIn(TRIG_PIN, HIGH, ECHO_TIMEOUT_US);
#endif

  if (duration == 0) {
    return NO_ECHO;
  }
  return (long) (duration / 58);
}

// NO_ECHO means the beam found nothing, which is the roomiest result there is
// -- so it has to compare as "very far", not as a negative number.
long clearance(long distanceCm) {
  return (distanceCm == NO_ECHO) ? FAR_AWAY_CM : distanceCm;
}

// Points the sensor at an angle, waits for the horn to settle, and measures.
long lookAt(int angle) {
  scanner.write(angle);
  delay(SERVO_SETTLE_MS);
  return readDistanceCm();
}

// --- The avoidance manoeuvre ------------------------------------------------

void avoidObstacle() {
  Serial.println(F("Obstacle within 10 cm -> STOP"));
  stopAll();
  delay(PAUSE_MS);

  Serial.println(F("  reversing"));
  backward();
  delay(BACK_OFF_MS);
  stopAll();
  delay(PAUSE_MS);

  long rightCm = lookAt(SCAN_RIGHT_DEG);
  long leftCm  = lookAt(SCAN_LEFT_DEG);
  lookAt(SCAN_CENTER_DEG);          // face forward again before driving on

  Serial.print(F("  free space  right: "));
  Serial.print(clearance(rightCm));
  Serial.print(F(" cm   left: "));
  Serial.print(clearance(leftCm));
  Serial.println(F(" cm"));

  if (clearance(rightCm) >= clearance(leftCm)) {
    Serial.println(F("  turning RIGHT"));
    turnRight();
  } else {
    Serial.println(F("  turning LEFT"));
    turnLeft();
  }
  delay(TURN_MS);

  stopAll();
  delay(PAUSE_MS);
}

// --- Program ----------------------------------------------------------------

void setup() {
  Serial.begin(9600);

  for (uint8_t i = 0; i < MOTOR_COUNT; i++) {
    pinMode(MOTOR_EN[i], OUTPUT);
    pinMode(MOTOR_A [i], OUTPUT);
    pinMode(MOTOR_B [i], OUTPUT);
  }
  stopAll();

#if ULTRASONIC_4_PIN
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
#endif

  scanner.attach(SERVO_PIN);
  scanner.write(SCAN_CENTER_DEG);   // sensor starts looking straight ahead
  delay(SERVO_SETTLE_MS);

  Serial.println(F("4WD obstacle avoider ready - driving forward."));
}

void loop() {
  long distanceCm = readDistanceCm();

  // NO_ECHO is "nothing in range", so only a real reading at or under 10 cm
  // counts as an obstacle.
  if (distanceCm != NO_ECHO && distanceCm <= STOP_DISTANCE_CM) {
    avoidObstacle();
  } else {
    forward();
  }

  delay(PING_PERIOD_MS);
}
