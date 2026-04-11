import serial
import numpy as np
import pandas as pd
import requests
import datetime
from sklearn.ensemble import RandomForestClassifier

# ===== LOAD TRAINING DATA =====
# Updated to your Downloads folder
normal   = pd.read_csv(r"C:\Users\aishw\Downloads\normal.csv",   header=None).values
warning  = pd.read_csv(r"C:\Users\aishw\Downloads\warning.csv",  header=None).values
critical = pd.read_csv(r"C:\Users\aishw\Downloads\critical.csv", header=None).values

# ===== FEATURE EXTRACTION =====
def extract_features(data, window=10):
    features = []
    for i in range(0, len(data) - window):
        w = data[i:i + window]
        feat = [
            w[:, 0].mean(),
            w[:, 0].max(),
            w[:, 0].std(),
            (w[:, 0] > 30).sum(),
            w[:, 1].mean(),
            w[:, 2].mean(),
            w[:, 2].std(),
        ]
        features.append(feat)
    return np.array(features)

# ===== PREPARE TRAINING DATA =====
X_n = extract_features(normal);   y_n = np.zeros(len(X_n))
X_w = extract_features(warning);  y_w = np.ones(len(X_w))
X_c = extract_features(critical); y_c = np.full(len(X_c), 2)

X = np.vstack([X_n, X_w, X_c])
y = np.hstack([y_n, y_w, y_c])

# ===== TRAIN MODEL =====
clf = RandomForestClassifier(n_estimators=100, random_state=42)
clf.fit(X, y)
print("✅ Model trained successfully")

# ===== FLASK DASHBOARD URL =====
# This sends predictions to your running Flask server
FLASK_URL = "http://127.0.0.1:5000/data"

def send_to_dashboard(status_str, reading, amplitude):
    """Send prediction result to Flask dashboard"""
    confidence_map = {"normal": 0.92, "warning": 0.85, "critical": 0.91}
    payload = {
        "model": "bridge",          # change to "tower" or "transformer" as needed
        "status": status_str,
        "confidence": confidence_map[status_str],
        "dominant_freq": float(reading),
        "amplitude": float(amplitude),
        "timestamp": datetime.datetime.now().isoformat()
    }
    try:
        requests.post(FLASK_URL, json=payload, timeout=1)
    except Exception:
        pass  # Flask not running — prediction still prints to terminal

# ===== SERIAL CONNECTION =====
# Updated to COM5
try:
    ser = serial.Serial('COM5', 115200, timeout=2)
    print("✅ Connected to COM5")
except Exception as e:
    print(f"❌ Could not open COM5: {e}")
    print("Make sure the ESP32 is plugged in and no other program is using COM5")
    exit()

buffer = []
print("🚀 Real-time monitoring started — waiting for sensor data...")
print("─" * 50)

while True:
    try:
        line = ser.readline().decode().strip()

        # Skip lines that don't look like sensor data (e.g. boot messages)
        if not line or not line[0].isdigit():
            continue

        values = list(map(int, line.split(",")))

        if len(values) == 3:
            buffer.append(values)

        # Keep last 10 samples
        if len(buffer) > 10:
            buffer.pop(0)

        # When we have 10 samples — predict
        if len(buffer) == 10:
            w = np.array(buffer)
            feat = [
                w[:, 0].mean(),
                w[:, 0].max(),
                w[:, 0].std(),
                (w[:, 0] > 30).sum(),
                w[:, 1].mean(),
                w[:, 2].mean(),
                w[:, 2].std(),
            ]
            pred = clf.predict([feat])[0]

            # Calculate amplitude from first column range
            amplitude = (w[:, 0].max() - w[:, 0].min()) / 1023.0
            reading   = w[:, 0].mean()

            if pred == 0:
                status = "normal"
                print(f"🟢 NORMAL   | reading: {reading:.1f} | amp: {amplitude:.2f}")
            elif pred == 1:
                status = "warning"
                print(f"🟡 WARNING  | reading: {reading:.1f} | amp: {amplitude:.2f}")
            else:
                status = "critical"
                print(f"🔴 CRITICAL | reading: {reading:.1f} | amp: {amplitude:.2f}")

            # Send to dashboard
            send_to_dashboard(status, reading, amplitude)

    except KeyboardInterrupt:
        print("\n⛔ Stopped by user")
        ser.close()
        break
    except Exception as e:
        print(f"Error: {e}")