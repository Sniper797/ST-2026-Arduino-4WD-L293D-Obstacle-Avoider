/*
 * ST-2026 - Electrical Task 2  |  Sketch 1 of 2: the timed motor sequence
 *
 * Smart Methods (ST 2026) summer training.
 *
 *   1. Forward  for 30 seconds
 *   2. Backward for 1 minute
 *   3. Right / left alternating for 1 minute
 *   ...then stop.
 *
 * Both sketches run on ONE circuit: four DC motors on two L293D drivers, plus
 * a servo carrying an ultrasonic sensor. This sketch only uses the motors --
 * the servo and the sensor simply sit idle while it runs. Sketch 2
 * (Task2_ObstacleAvoider) uses all of it.
 *
 * One L293D contains two H-bridges, so it drives two DC motors. Four motors
 * therefore need two chips: U1 carries the front axle, U2 the rear axle.
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
 *   Servo signal -> D10 and ultrasonic SIG -> A2 are wired for sketch 2 and
 *   are simply never touched here.
 *
 *   In Tinkercad the board's 5V rail drives all four motors fine. On real
 *   hardware feed VCC2 (pin 8) from a separate battery pack and tie its
 *   ground to the Arduino ground -- four motors stalling can pull several
 *   amps, which the USB-powered 5V pin cannot deliver.
 */

// --- Motor identity ---------------------------------------------------------

const uint8_t FRONT_LEFT  = 0;
const uint8_t FRONT_RIGHT = 1;
const uint8_t REAR_LEFT   = 2;
const uint8_t REAR_RIGHT  = 3;
const uint8_t MOTOR_COUNT = 4;

// --- Pin map ----------------------------------------------------------------
// Indexed FRONT_LEFT, FRONT_RIGHT, REAR_LEFT, REAR_RIGHT.
// The four enables must land on PWM pins, and 9 and 10 are deliberately left
// out: in sketch 2 the Servo library takes over Timer1 and disables
// analogWrite() on exactly those two pins. Keeping the enables on 3/5/6/11
// means one wiring serves both sketches.

const uint8_t MOTOR_EN[MOTOR_COUNT] = { 3,  5,  6, 11};  // L293D 1,2EN / 3,4EN
const uint8_t MOTOR_A [MOTOR_COUNT] = { 2,  7, 12, A0};  // L293D 1A / 3A
const uint8_t MOTOR_B [MOTOR_COUNT] = { 4,  8, 13, A1};  // L293D 2A / 4A

// On a real chassis the right-hand motors are mounted mirrored, so the same
// voltage spins them the opposite way. Set their flag to true and "forward"
// becomes forward for all four. In the Tinkercad simulation the motors are
// loose on the canvas, so all four are left unflipped and spin alike.
//
// Tinkercad also has its own rpm sign convention, so "forward" may well read
// as negative rpm. That is not a direction error -- what matters is that all
// four carry the same sign and flip together. Set all four flags to true if
// you would rather see forward read positive.
const bool MOTOR_FLIP[MOTOR_COUNT] = {false, false, false, false};

// --- Timing (the task) ------------------------------------------------------

const unsigned long FORWARD_MS    = 30000UL;  // 30 seconds
const unsigned long BACKWARD_MS   = 60000UL;  // 1 minute
const unsigned long ALTERNATE_MS  = 60000UL;  // 1 minute of turning
const unsigned long TURN_SLICE_MS =  5000UL;  // 5 s right, 5 s left, repeat

const uint8_t DRIVE_SPEED = 200;  // 0..255, the PWM duty on the enable pins
const uint8_t TURN_SPEED  = 160;  // a little slower so the turn stays readable

// --- Low-level motor control ------------------------------------------------

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

// Drives the whole robot: one signed speed for each side of the chassis.
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

// --- Movement 3: right and left, alternating --------------------------------

// Turns right, then left, then right... in TURN_SLICE_MS slices until
// totalMs have elapsed. The final slice is trimmed so the phase ends on time
// even when totalMs is not a whole number of slices.
void alternateTurns(unsigned long totalMs) {
  const unsigned long startTime = millis();
  bool goRight = true;

  while (millis() - startTime < totalMs) {
    unsigned long elapsed   = millis() - startTime;
    unsigned long remaining = totalMs - elapsed;
    unsigned long slice     = (remaining < TURN_SLICE_MS) ? remaining : TURN_SLICE_MS;

    if (goRight) {
      Serial.println(F("   turning RIGHT"));
      turnRight();
    } else {
      Serial.println(F("   turning LEFT"));
      turnLeft();
    }

    delay(slice);
    goRight = !goRight;
  }
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

  Serial.println(F("1) FORWARD for 30 s"));
  forward();
  delay(FORWARD_MS);

  Serial.println(F("2) BACKWARD for 60 s"));
  backward();
  delay(BACKWARD_MS);

  Serial.println(F("3) RIGHT / LEFT alternating for 60 s"));
  alternateTurns(ALTERNATE_MS);

  Serial.println(F("Sequence complete - motors stopped."));
  stopAll();
}

void loop() {
  // Nothing to do. The sequence runs once; press Reset to run it again.
}
