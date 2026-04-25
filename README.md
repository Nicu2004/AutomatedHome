# 🏠 AutomatedHome

A personal home automation project built with two Arduino boards communicating over SoftwareSerial. The system automates window control, fan, water pump, and LED lighting based on real-time sensor data, with support for manual override via Serial commands.

---

## 📋 Table of Contents

- [How It Works](#how-it-works)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Wiring Overview](#wiring-overview)
- [Setup & Installation](#setup--installation)
- [Serial Commands](#serial-commands)
- [Usage](#usage)
- [Project Structure](#project-structure)
- [Contributing](#contributing)

---

## How It Works

The project uses two Arduino boards:

### 🧠 Master Board (`scketchMaster.ino`)
The master handles the **front door gate** and user interface:
- Reads distance via an **HC-SR04 ultrasonic sensor** — automatically opens the gate (servo motor) when something is within 21 cm for 1.5 seconds, and closes it when the object moves away past 35 cm
- Displays temperature, humidity, and gate status on a **16x2 LCD**
- Accepts Serial commands (`m`, `a`, `b`, `l`) to switch between auto/manual mode and control motors
- Communicates motor commands to the slave board every second (or on change) via SoftwareSerial (pins A4/A5)
- Receives temperature and soil moisture data back from the slave to display on LCD

### ⚙️ Slave Board (`slaveCode.ino`)
The slave handles **environmental automation** inside the home:
- Reads **temperature & humidity** (DHT11) every 2 seconds
- Automatically opens/closes a **window** (stepper motor) based on temperature:
  - Opens if temperature is between 25°C and 30°C
  - Closes otherwise
- Controls a **fan** (Motor A) — turns on automatically if temperature exceeds 30°C
- Controls a **water pump** (Motor B) — activates automatically if soil moisture sensor reads above 800
- Adjusts an **LED** brightness automatically based on ambient light (photoresistor)
- Sends temperature and soil moisture data back to the master every 2 seconds
- Supports manual override commands (`o`, `c`, `r`) via Serial Monitor

---

## Hardware Requirements

| Component | Quantity | Used By | Purpose |
|---|---|---|---|
| Arduino Board (Uno/Mega) | 2 | Both | Main controllers |
| HC-SR04 Ultrasonic Sensor | 1 | Master | Gate distance detection |
| Servo Motor | 1 | Master | Gate open/close |
| 16x2 LCD Display | 1 | Master | Status display |
| Buzzer | 1 | Master | Command feedback beep |
| LED (indicator) | 1 | Master | LED toggle via `l` command |
| DHT11 Temperature/Humidity Sensor | 1 | Slave | Temp & humidity readings |
| Stepper Motor (4-wire, 250 steps) | 1 | Slave | Window open/close |
| DC Motor x2 (with L298N or similar) | 2 | Slave | Fan (Motor A) & Pump (Motor B) |
| Photoresistor (LDR) | 1 | Slave | Ambient light sensing |
| Soil Moisture Sensor | 1 | Slave | Plant watering detection |
| LED (dimmable) | 1 | Slave | Auto brightness based on light |
| Jumper Wires | Many | Both | Connections |
| Breadboard | 1-2 | Both | Prototyping |

---

## Software Requirements

- [Arduino IDE](https://www.arduino.cc/en/software) (v1.8+ or v2.x)
- Required Libraries (install via Arduino Library Manager → **Sketch > Include Library > Manage Libraries**):

| Library | Used By | Install Name |
|---|---|---|
| `NewPing` | Master | `NewPing` |
| `LiquidCrystal` | Master | Built-in |
| `Servo` | Master | Built-in |
| `SoftwareSerial` | Both | Built-in |
| `DHT11` | Slave | `DHT11` |
| `Stepper` | Slave | Built-in |

---

## Wiring Overview

### Master Board Pin Map

| Pin | Component |
|---|---|
| 8 | Ultrasonic TRIG |
| 10 | Ultrasonic ECHO |
| 9 | Servo Motor |
| 12 | Buzzer |
| A0 | LED indicator |
| A4 | SoftwareSerial RX (to Slave TX) |
| A5 | SoftwareSerial TX (to Slave RX) |
| 7, 6, 5, 4, 3, 11 | LCD (RS, E, D4, D5, D6, D7) |

### Slave Board Pin Map

| Pin | Component |
|---|---|
| 10 | SoftwareSerial RX (to Master TX) |
| 11 | SoftwareSerial TX (to Master RX) |
| 2 | DHT11 Sensor |
| A0 | Photoresistor |
| A1 | Soil Moisture Sensor |
| 3 | Dimmable LED |
| 4, 5, 6 | Motor A (Fan) — lftA, rgtA, pwmA |
| 7, 8, 9 | Motor B (Pump) — lftB, rgtB, pwmB |
| A2, A3, A4, A5 | Stepper Motor |

> **Board-to-board connection:** Master A4 (RX) → Slave pin 11 (TX), and Master A5 (TX) → Slave pin 10 (RX). Share a common GND between both boards.

---

## Setup & Installation

### 1. Clone the repository

```bash
git clone https://github.com/Nicu2004/AutomatedHome.git
cd AutomatedHome
```

### 2. Install required libraries

Open Arduino IDE → **Sketch > Include Library > Manage Libraries**, then search for and install:
- `NewPing`
- `DHT11`

### 3. Upload to each board

- Open `scketchMaster.ino` → select your **Master Arduino** board and port → click **Upload**
- Open `slaveCode.ino` → select your **Slave Arduino** board and port → click **Upload**

### 4. Wire everything up

Connect components according to the pin map above and link the two boards via SoftwareSerial (with shared GND).

### 5. Power up

Open the Serial Monitor at **9600 baud** on each board to see live output.

---

## Serial Commands

### Master Board (via Serial Monitor at 9600 baud)

| Command | Action |
|---|---|
| `m` | Toggle manual / auto motor mode |
| `a` | Toggle Motor A (fan) on/off — switches to manual |
| `b` | Toggle Motor B (pump) on/off — switches to manual |
| `l` | Toggle the indicator LED |

### Slave Board (via Serial Monitor at 9600 baud)

| Command | Action |
|---|---|
| `o` | Manually open the window (stepper) |
| `c` | Manually close the window (stepper) |
| `r` | Resume automatic window control based on temperature |

---

## Usage

Once both boards are running:

- **Walk toward the gate** → ultrasonic sensor detects proximity → servo opens the gate automatically after 1.5 seconds
- **Move away** → gate closes automatically
- **Temperature 25–30°C** → window opens automatically via stepper
- **Temperature > 30°C** → fan also turns on
- **Soil moisture > 800** → water pump activates
- **Ambient light drops** → LED brightens automatically
- **LCD** shows gate status and live temperature/humidity from the slave

**Example Master Serial output:**
```
System Ready. Monitoring Distance...
Distance: 15 cm
Distance: 12 cm
Distance: Out of Range
```

**Example Slave Serial output:**
```
Slave Ready. Cmds: 'c'=Close, 'o'=Open, 'r'=Resume Auto
Auto: Opening Window...
Auto: Closing Window...
```

---

## Project Structure

```
AutomatedHome/
├── scketchMaster.ino   # Master board — gate, LCD, serial commands, board communication
├── slaveCode.ino       # Slave board — sensors, window, fan, pump, LED
└── README.md
```

---

## Contributing

This is a personal project, but suggestions are welcome!

1. Fork the repository
2. Create a new branch: `git checkout -b feature/your-feature`
3. Commit your changes: `git commit -m "Add your feature"`
4. Push to the branch: `git push origin feature/your-feature`
5. Open a Pull Request

---

> Built by 4 with ❤️ and two Arduinos
