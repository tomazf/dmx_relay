# 🎛️ DMX Relay Controller

An Arduino-based DMX controller for triggering RGB Par lamps and relay outputs via a simple two-button interface. Designed for industrial and maker applications where a physical start/stop control is needed to drive DMX lighting scenes and relay-switched loads.

---

## ✨ Features

- 🔘 Two-button control (Start/Stop)
- ✨ Random blink sequence before activation (0.5 – 2.5 sec, configurable)
- 💡 Controls multiple RGB Par lamps over DMX
- ⚡ Controls multiple DMX relay outputs
- 🟢 Status LEDs (green = active, red = idle/blinking)
- 🔒 Debounced button inputs with hardware RC filter
- 💾 DMX write caching — only writes when scene actually changes

---

## 🔧 Hardware

### Components

| Component | Description |
|---|---|
| Arduino Nano (or compatible) | Main controller |
| DMX GYF-838 module (SP3485) | DMX-512 output |
| ULN2003A | LED driver |
| 2× momentary pushbutton | Start/Stop |
| LEDs | Status indicators |

---

### 📍 Pin Assignment

| Pin | Function |
|---|---|
| D2 | Green LED 1 |
| D3 | Green LED 2 |
| D4 | Red LED |
| D11 | Stop button |
| D12 | Start button |
| D13 | Debug LED (onboard) |
| HW TX (D1) | DMX output |

---

### 🔌 Wiring Overview

#### Buttons (RC filter + external pull-up, active LOW)

Each button input uses a small RC filter combined with a pull-up resistor for clean, noise-free readings. The 1kΩ series resistor and 10nF capacitor form a low-pass filter that suppresses high-frequency noise and contact bounce at the hardware level, complementing the software debouncing done by the Bounce2 library.

```
 +5V
  |
 100kΩ  (pull-up)
  |
  +----> 10nF ----> GND  (filter capacitor)
  +----> Arduino input pin (D11 or D12)
  |
 1kΩ    (series resistor)
  |
 [BTN]
  |
 GND
```

Button is normally HIGH. When pressed, pulls the pin LOW → Bounce2 library detects `fell()`.

---

#### 💡 LEDs via ULN2003A (common anode, active LOW)

The ULN2003A is an NPN Darlington array — it sinks current. With common anode LEDs,
the anode is tied to +5V and the cathode is driven LOW through the ULN2003A to turn the LED ON.


---

## 🔄 State Machine

The controller runs a three-state machine:

```
                  [START pressed]
  STATE_IDLE  ─────────────────────>  STATE_BLINKING
      ^                                     |
      |                                     | random delay expires between
      |                               (0.5 – 2.5 sec)
      |                                     |
      |           [STOP pressed]            v
      +<────────────────────────────  STATE_ACTIVE
```

### State Descriptions

| State | 🔴 Red LED | 🟢 Green LEDs | DMX Output |
|---|---|---|---|
| `STATE_IDLE` | ON | OFF | All channels OFF |
| `STATE_BLINKING` | ON | blinking | All channels OFF |
| `STATE_ACTIVE` | OFF | solid ON | RGB green + relays ON |

**STATE_IDLE** — System is waiting. Red LED on, all DMX outputs off.

**STATE_BLINKING** — Triggered by Start button. Green LEDs blink rapidly for a random duration before activation. Adds uncertainty/anticipation before the load switches on.

**STATE_ACTIVE** — RGB lamps set to green, relay outputs set to full (255). Stop button returns to IDLE from any non-idle state.

---

## 📦 Dependencies

Install via Arduino Library Manager:

- [Bounce2](https://github.com/thomasfredericks/Bounce2) — button debouncing
- [DMXSerial](https://github.com/mathertel/DMXSerial) — DMX-512 output

Also requires the custom `ledBlink` class (`src/ledBlink.h`) included in this repository.

---

## 📄 License

MIT License — free to use, modify, and distribute.
