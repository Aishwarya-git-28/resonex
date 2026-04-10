# RESONEX — Non-Invasive Acoustic Sensing for Infrastructure Monitoring

> **FantomCode 2026 — 24 Hour National Level Hackathon**
> Team Name: **Visionary Loop**

---

## Team Members

| Name | Role |
|---|---|
| Aishwarya M | Hardware + Firmware + Software Integration |
| Dhruthi Salankimatt | ML Model Training (1D-CNN) |
| Chandana P | Dashboard Development |
| Sheela K Oli | Project Lead + Documentation |

---

## The Problem

Critical infrastructure — power transformers, bridges, and towers — forms the backbone of modern cities. These systems fail **internally** before showing any surface damage. Current inspection methods require:

- Shutting down operations
- Manual physical inspections
- Expensive diagnostic equipment
- Periodic scheduling regardless of actual condition

This means faults are either caught too late (after failure) or inspections waste resources on healthy equipment.

**RESONEX solves this by listening.**

---

## What is RESONEX?

RESONEX is a **non-invasive acoustic monitoring system** that attaches a small piezoelectric contact sensor to the outside of any structure — no disassembly, no shutdown, no modification.

It continuously listens to internal vibrations, analyzes them using a **1D Convolutional Neural Network**, and classifies the health of the structure in real time as:

- **Normal** — operating within expected parameters
- **Warning** — anomalous patterns detected, monitor closely
- **Critical** — fault signature detected, immediate action required

Everything runs **on the edge** — no cloud dependency, works offline.

---

## How It Works

```
Piezo Sensor → Signal Conditioning → ESP32 ADC → 1D-CNN Inference → WiFi → Live Dashboard
```

1. Piezoelectric contact sensor picks up vibrations from the structure surface
2. LM358 op-amp amplifies the millivolt signal to a readable range
3. ESP32 samples at 2kHz, buffers 512 readings per window
4. Signal is classified as Normal / Warning / Critical
5. LEDs and buzzer provide immediate local feedback
6. Reading is sent over WiFi as JSON to the Flask dashboard
7. Dashboard displays live status, trend charts, alert log, and replacement analysis

---

## Three Prototype Models

| Model | Simulates | Sensor Used | Fault Simulated |
|---|---|---|---|
| Transformer | Power transformer | Piezo + Mic | Internal motor vibration, frequency drift |
| Bridge | Road/rail bridge | Piezo | Structural loosening, fatigue vibration |
| Tower | Transmission tower | Piezo + MPU6050 | Sway, vibration anomaly |

---

## Tech Stack

| Layer | Technology |
|---|---|
| Sensors | Piezoelectric disc sensor, MPU6050 accelerometer/gyro, Electret mic |
| Signal conditioning | LM358 dual op-amp |
| Edge processor | ESP32 development board |
| Firmware | Arduino C++ |
| AI model | 1D-CNN — TensorFlow Keras → TensorFlow Lite (TinyML) |
| Backend | Python Flask |
| Frontend | HTML5, CSS3, JavaScript, Chart.js |
| Version control | Git + GitHub |

---

## Repository Structure

```
resonex/
├── firmware/
│   └── resonex.ino          # ESP32 Arduino firmware
├── model/
│   ├── train.py             # 1D-CNN training script (Google Colab)
│   └── README.md            # Model training instructions
├── templates/
│   └── index.html           # Live monitoring dashboard
├── static/                  # Static assets
├── data/
│   └── .gitkeep             # Sensor recordings go here (gitignored)
├── app.py                   # Flask server
├── mock_esp32.py            # Hardware simulator for testing without hardware
├── collect_data.py          # Serial data collection script
├── .gitignore
└── README.md
```

---

## Hardware Components

| Component | Purpose | Quantity |
|---|---|---|
| ESP32 development board | Edge AI brain, WiFi, ADC | 1 |
| Piezoelectric disc sensor 27mm | Contact vibration sensing | 3 |
| MPU6050 accelerometer/gyro | Tower structural movement | 1 |
| Electret mic module | Airborne acoustic sensing | 1 |
| LM358 op-amp IC | Signal amplification | 2 |
| Green / Yellow / Red LEDs | Status indicators | 3 |
| Passive buzzer module | Critical alert sound | 1 |
| Breadboard + jumper wires | Circuit assembly | — |

---

## Circuit Connections

| Component | ESP32 Pin |
|---|---|
| Piezo sensor (positive) | GPIO 34 |
| Piezo sensor (negative) | GND |
| MPU6050 SDA | GPIO 21 |
| MPU6050 SCL | GPIO 22 |
| Green LED | GPIO 25 |
| Yellow LED | GPIO 26 |
| Red LED | GPIO 27 |
| Buzzer | GPIO 15 |

---

## Running the Project

### Option 1 — With mock data (no hardware needed)

```bash
# Install dependencies
pip install flask requests numpy

# Terminal 1 — start Flask server
python app.py

# Terminal 2 — start hardware simulator
python mock_esp32.py
```

Open browser at `http://localhost:5000`

### Option 2 — With real hardware

1. Open `firmware/resonex.ino` in Arduino IDE
2. Update WiFi credentials and laptop IP at the top of the file:
```cpp
const char* WIFI_SSID     = "YourWiFiName";
const char* WIFI_PASSWORD = "YourPassword";
const char* SERVER_URL    = "http://YOUR_LAPTOP_IP:5000/data";
```
3. Flash to ESP32 — Tools → Board → ESP32 Dev Module → Upload
4. Run `python app.py` on your laptop
5. Open `http://localhost:5000`

---

## AI Model — 1D-CNN Architecture

```
Input (window_size=20, features=3)
    ↓
Conv1D (32 filters, kernel=3, ReLU)
    ↓
MaxPooling1D (pool=2)
    ↓
Conv1D (64 filters, kernel=3, ReLU)
    ↓
MaxPooling1D (pool=2)
    ↓
Flatten
    ↓
Dense (64, ReLU)
    ↓
Dense (3, Softmax) → [Normal, Warning, Critical]
```

**Training:**
- Data collected from real hardware sensors
- 3 classes: Normal (label 0), Warning (label 1), Critical (label 2)
- Normalized to 0–1 range (÷ 4095 for 12-bit ADC)
- Sliding window of 20 samples, step size 5
- 80/20 train/test split
- 30 epochs, Adam optimizer, sparse categorical crossentropy loss

**Deployment:**
- Trained model saved as `.keras`
- Converted to TensorFlow Lite for ESP32 deployment
- Inference runs entirely on-device — no internet required

---

## Dashboard Features

### Dashboard page
- Live status for all 3 models simultaneously
- 4 model selector tabs (All / Transformer / Bridge / Tower)
- Real-time sensor reading trend chart
- Status distribution doughnut chart
- Last 40 readings status history bar
- Critical alert popup with beep sound
- Persistent alert counter and numbered alert log
- Live event log

### Analysis page
- Per-model fault rate cards with replacement recommendations
- Bridge and Tower flagged as non-renovatable — system recommends replacement vs repair
- Stacked bar chart: normal / warning / critical counts per model
- Fault rate trend line — rising trend = structural deterioration

### History page
- Full filterable event history
- Filter by model: All / Transformer / Bridge / Tower
- Filter by status: All / Critical only / Warning only / Normal only
- Summary stats update based on active filter

---

## Data Flow

```
ESP32 Hardware                    Laptop
─────────────                     ──────
analogRead(GPIO34)
    ↓
Buffer 512 samples @ 2kHz
    ↓
Calculate avg, peak-to-peak,
amplitude
    ↓
Classify: Normal/Warning/Critical
    ↓
Set LEDs + Buzzer
    ↓
HTTP POST JSON ──────────────→  Flask /data endpoint
                                    ↓
                                Store in deque (last 100)
                                    ↓
Browser polls /feed ←───────── GET /feed every 1 second
    ↓
Dashboard updates live
```

---

## JSON Data Contract

All data between ESP32 and dashboard follows this format:

```json
{
  "model": "transformer",
  "status": "warning",
  "confidence": 0.91,
  "dominant_freq": 142.3,
  "amplitude": 0.63,
  "timestamp": "live"
}
```

---

## SDG Alignment

| SDG | How RESONEX contributes |
|---|---|
| **SDG 7** — Affordable and Clean Energy | Protects power transformers from unexpected failure, preventing grid outages |
| **SDG 9** — Industry, Innovation and Infrastructure | Enables resilient, continuously monitored infrastructure at low cost |
| **SDG 11** — Sustainable Cities and Communities | Prevents bridge and tower failures that endanger urban populations |

---

## Impact

| Metric | Value |
|---|---|
| Prototype BOM cost | Under ₹3,000 |
| Typical commercial solution cost | ₹3,00,000 – ₹40,00,000 |
| Installation required | External attachment only — no shutdown |
| Internet dependency | None — full edge inference |
| Assets protected per unit | 1 structure (scalable with multiplexing) |

---

## Future Scope

- TFLite model deployment for full on-device CNN inference
- Multi-sensor multiplexing — one ESP32 monitoring multiple structures
- LoRaWAN connectivity for remote infrastructure without WiFi
- Mobile app for field engineer alerts
- Ghost Worker Initiative — extending to urban tree fall prediction

---

## License

Built for FantomCode 2026. All rights reserved by Team Visionary Loop.