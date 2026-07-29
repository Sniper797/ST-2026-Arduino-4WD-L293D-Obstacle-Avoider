# Wiring — one circuit for both sketches

**Arduino Uno + 2× L293D + 4 DC motors + servo + ultrasonic.** One Tinkercad design
carries everything; the two sketches run on it unchanged.

- **Sketch 1** (`Task1_FourMotorSequence`) uses only the motors — the servo and the
  sensor sit idle while it runs.
- **Sketch 2** (`Task2_ObstacleAvoider`) uses all of it.

Nothing has to be rewired between them.

---

## 0. Before you start: reading an L293D

The L293D is a 16-pin DIP. Find the **notch / white dot** on one end — that end is
the pin-1 end. With the dot on the **left**:

```
          16  15  14  13  12  11  10   9
          |   |   |   |   |   |   |   |
      +---------------------------------+
  (•)  |            L293D                |
      +---------------------------------+
          |   |   |   |   |   |   |   |
           1   2   3   4   5   6   7   8
```

Pin **1** is bottom-left, pins 1→8 run left-to-right along the bottom, then pin **9**
is top-right and pins 9→16 run right-to-left along the top. (Counter-clockwise from
pin 1 — the standard for every DIP chip.)

> 💡 In Tinkercad you don't have to count: **hover over any pin and its name appears**
> (`1,2EN`, `1A`, `1Y`, `GND`, `VCC2` …). Wire by name using the tables below — it is
> far less error-prone than counting legs.

**What each pin does:**

| Name | Meaning |
|---|---|
| `VCC1` (16) | Logic supply — 5 V, powers the chip's brain |
| `VCC2` (8) | **Motor** supply — the voltage that actually reaches the motors |
| `1,2EN` (1) | Enable for channel A. LOW = motor off. PWM here = speed control |
| `3,4EN` (9) | Enable for channel B |
| `1A`/`2A` (2, 7) | Direction inputs for motor A — one HIGH, one LOW picks the direction |
| `3A`/`4A` (10, 15) | Direction inputs for motor B |
| `1Y`/`2Y` (3, 6) | Outputs to motor A's two terminals |
| `3Y`/`4Y` (11, 14) | Outputs to motor B's two terminals |
| `GND` (4, 5, 12, 13) | Ground — **wire all four**, they are also the heat sink |

**One chip = two motors.** That is why four motors need two chips.

---

## 1. Parts list

| Qty | Part |
|---|---|
| 1 | Arduino Uno R3 |
| 2 | L293D |
| 4 | DC Motor |
| 1 | Micro Servo (SG90) |
| 1 | Ultrasonic Distance Sensor — **PING)))**, the 3-pin one |
| 1 | Breadboard (small is enough) |

---

## 2. Power rails (do this first)

Park the two L293D chips across the breadboard's centre channel, then:

| From | To |
|---|---|
| Arduino **5V** | breadboard **+** (red) rail |
| Arduino **GND** | breadboard **−** (black) rail |

And give **both** chips power:

| L293D pin | Name | Goes to |
|---|---|---|
| 16 | `VCC1` | **+** rail (5 V) |
| 8 | `VCC2` | **+** rail (5 V) |
| 4, 5, 12, 13 | `GND` | **−** rail (GND) |

> ⚠️ **Real hardware:** `VCC2` (pin 8) should come from a separate battery pack
> (6–9 V), *not* the Arduino's 5V pin — four motors stalling can pull several amps and
> will brown-out a USB-powered board, and a browning-out board makes the servo jitter
> too. Tie the pack's ground to the Arduino ground so they share a reference. In the
> Tinkercad simulator the 5V rail is ideal, so 5 V is fine.
>
> The L293D also drops about **1.5–2 V** internally, so a motor fed from 5 V sees
> roughly 3–3.5 V. Perfectly normal — it is a bipolar chip, not a MOSFET driver.

---

## 3. U1 — the front axle (front-left + front-right motors)

| L293D pin | Name | Connects to |
|---|---|---|
| 1 | `1,2EN` | Arduino **D3** *(PWM — front-left speed)* |
| 2 | `1A` | Arduino **D2** |
| 3 | `1Y` | **Front-left motor**, terminal 1 |
| 6 | `2Y` | **Front-left motor**, terminal 2 |
| 7 | `2A` | Arduino **D4** |
| 9 | `3,4EN` | Arduino **D5** *(PWM — front-right speed)* |
| 10 | `3A` | Arduino **D7** |
| 11 | `3Y` | **Front-right motor**, terminal 1 |
| 14 | `4Y` | **Front-right motor**, terminal 2 |
| 15 | `4A` | Arduino **D8** |

## 4. U2 — the rear axle (rear-left + rear-right motors)

| L293D pin | Name | Connects to |
|---|---|---|
| 1 | `1,2EN` | Arduino **D6** *(PWM — rear-left speed)* |
| 2 | `1A` | Arduino **D12** |
| 3 | `1Y` | **Rear-left motor**, terminal 1 |
| 6 | `2Y` | **Rear-left motor**, terminal 2 |
| 7 | `2A` | Arduino **D13** |
| 9 | `3,4EN` | Arduino **D11** *(PWM — rear-right speed)* |
| 10 | `3A` | Arduino **A0** |
| 11 | `3Y` | **Rear-right motor**, terminal 1 |
| 14 | `4Y` | **Rear-right motor**, terminal 2 |
| 15 | `4A` | Arduino **A1** |

> `A0` and `A1` are used here as plain **digital** outputs. Every analog pin on the Uno
> doubles as a digital pin, and `digitalWrite(A0, HIGH)` works exactly as it does on D2.
> They are needed because 12 motor signals + a servo + a sensor is more than the twelve
> usable digital pins (D2–D13) can hold.

---

## 5. Servo and ultrasonic

Only sketch 2 uses these, but wire them now — sketch 1 simply never touches them.

**Servo motor** — the sensor's neck. Mount the sensor on its horn so that turning the
servo aims the sensor.

| Servo wire | Goes to |
|---|---|
| orange / yellow — **signal** | Arduino **D10** |
| red — power | **+** rail (5 V) |
| brown / black — ground | **−** rail (GND) |

**Ultrasonic PING)))** — the 3-pin Parallax sensor, Tinkercad's default:

| Sensor pin | Goes to |
|---|---|
| `SIG` | Arduino **A2** |
| `5V` | **+** rail (5 V) |
| `GND` | **−** rail (GND) |

> Using the **4-pin HC-SR04** instead? Set `#define ULTRASONIC_4_PIN 1` at the top of
> sketch 2, then wire `TRIG → A2` and `ECHO → A3`.

### The servo does not go through the L293D — and that is correct

A servo **cannot** be driven by an H-bridge. It is not a bare DC motor: it contains its
own H-bridge, a potentiometer and a control chip, and it expects a PWM *signal* on a
single wire.

That is not a workaround for Tinkercad — it is what the real hardware does. The
Adafruit / Deek-Robot **L293D shield** carries two 3-pin servo headers, and those
headers are hard-wired straight to Arduino pins **10** and **9**, bypassing the L293D
chips completely. Plugging a servo "into the L293D shield" is electrically identical to
plugging it into a digital pin. **D10 here is the shield's `SERVO_1` pin.**

### Why the enables are 3, 5, 6, 11 — never 9 or 10

The `Servo` library takes over the Uno's **Timer1**, which permanently disables
`analogWrite()` on **pins 9 and 10** for as long as a servo is attached. Keeping all
four enable pins off 9 and 10 means attaching the servo costs no PWM speed control at
all, and one wiring serves both sketches. Pin 9 stays free as a spare.

---

## 6. The full Arduino pin map

| Arduino pin | Used for | Sketch 1 | Sketch 2 |
|---|---|:--:|:--:|
| **D2** | U1 `1A` — front-left direction | ✅ | ✅ |
| **D3** ~ | U1 `1,2EN` — front-left enable / speed | ✅ | ✅ |
| **D4** | U1 `2A` — front-left direction | ✅ | ✅ |
| **D5** ~ | U1 `3,4EN` — front-right enable / speed | ✅ | ✅ |
| **D6** ~ | U2 `1,2EN` — rear-left enable / speed | ✅ | ✅ |
| **D7** | U1 `3A` — front-right direction | ✅ | ✅ |
| **D8** | U1 `4A` — front-right direction | ✅ | ✅ |
| **D9** ~ | *free* | | |
| **D10** ~ | **Servo signal** | | ✅ |
| **D11** ~ | U2 `3,4EN` — rear-right enable / speed | ✅ | ✅ |
| **D12** | U2 `1A` — rear-left direction | ✅ | ✅ |
| **D13** | U2 `2A` — rear-left direction | ✅ | ✅ |
| **A0** | U2 `3A` — rear-right direction | ✅ | ✅ |
| **A1** | U2 `4A` — rear-right direction | ✅ | ✅ |
| **A2** | **Ultrasonic `SIG`** | | ✅ |
| **A3–A5** | *free* | | |
| **5V / GND** | breadboard rails | ✅ | ✅ |

`~` marks a hardware-PWM pin. D0 and D1 are left alone — they are the USB serial lines,
and both sketches print to the Serial Monitor at 9600 baud.

---

## 7. Wiring checklist

Before pressing **Start Simulation**:

- [ ] Both chips have `VCC1` **and** `VCC2` on 5 V
- [ ] All **four** `GND` pins on **both** chips reach the ground rail
- [ ] Arduino `GND`, servo ground and sensor `GND` all reach that **same** rail
- [ ] Each motor's two terminals go to a `Y` **pair** from the same channel
      (`1Y`+`2Y`, or `3Y`+`4Y`) — never to two different channels
- [ ] The four enable pins are on D3, D5, D6, D11 — **none on 9 or 10**
- [ ] Servo **signal** on **D10**, not on any L293D output
- [ ] Sensor `SIG` on **A2**
- [ ] The sensor is mounted on the servo horn, so it turns with it

---

## 8. How to test it

**Sketch 1** — paste it in, **Start Simulation**, and watch the clock:

| Time | What should happen |
|---|---|
| 0:00 → 0:30 | all four motors spin one way; Serial says `1) FORWARD for 30 s` |
| 0:30 → 1:30 | all four flip together; Serial says `2) BACKWARD for 60 s` |
| 1:30 → 2:30 | left and right pairs oppose, swapping every 5 s |
| 2:30 | everything stops |

> **On the rpm sign.** Tinkercad may show *negative* rpm during the forward phase and
> positive during backward. That is its own convention for which motor terminal sits at
> the higher voltage — not a direction error. What matters is that all four motors carry
> the **same sign** and **flip together**. To make forward read positive instead, set
> every entry of `MOTOR_FLIP[]` to `true` in the sketch.

**Sketch 2** — paste it in, **Start Simulation**:

1. All four motors drive forward, the servo holds at 90°.
2. Click the ultrasonic sensor — Tinkercad shows a **distance slider**.
3. Drag it below **10 cm**. The motors should stop, reverse briefly, the servo should
   sweep right → left → centre, and the robot should turn toward the side you left more
   room on.
4. The **Serial Monitor** (9600 baud) prints each decision and both measured distances.

---

## 9. Common mistakes

| Symptom | Cause |
|---|---|
| Motors never turn | `VCC2` (pin 8) not connected, or the enable pin left floating |
| Motors turn very weakly | Only `VCC1` powered — that feeds the logic, not the motors |
| One motor ignores direction | Its two terminals landed on different channels (e.g. `1Y` + `3Y`) |
| One motor's rpm sign disagrees with the other three | That motor's two wires are swapped — harmless in the sim, but it would fight the others on a real chassis |
| Servo does not move at all | Signal wire on an L293D output instead of **D10** |
| Servo jitters, motors stutter | Missing common ground between the supplies |
| Robot brakes at nothing | `SIG` on the wrong pin — a silent sensor reads as `0`, see the `NO_ECHO` note in the sketch |
| Distance never changes | Sensor `GND` not tied to the Arduino ground |
| Speed control does nothing | An enable pin sitting on pin 9 or 10 — the servo killed their PWM |
