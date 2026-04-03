import serial
import csv
import time
import datetime
import os

# Change this to your ESP32's COM port when hardware arrives
# On Windows it'll be something like COM3 or COM4
# Check Device Manager to find the right one
SERIAL_PORT = "COM3"
BAUD_RATE = 115200
LABEL = "normal"  # change to: normal / warning / critical
SAVE_DIR = "data"

os.makedirs(SAVE_DIR, exist_ok=True)

filename = f"{SAVE_DIR}/{LABEL}_{datetime.datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

print(f"Recording '{LABEL}' data to {filename}")
print("Press Ctrl+C to stop recording\n")

try:
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(["timestamp", "piezo1", "piezo2", "piezo3", "accel_x", "accel_y", "accel_z", "label"])
        
        count = 0
        while True:
            line = ser.readline().decode('utf-8').strip()
            if line:
                writer.writerow([datetime.datetime.now().isoformat()] + line.split(',') + [LABEL])
                count += 1
                if count % 100 == 0:
                    print(f"  {count} samples recorded...")

except KeyboardInterrupt:
    print(f"\nDone. {count} samples saved to {filename}")
except serial.SerialException as e:
    print(f"Could not open port {SERIAL_PORT}: {e}")
    print("Hardware not connected — this script only runs with real ESP32")