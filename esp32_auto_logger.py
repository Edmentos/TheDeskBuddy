import serial
import serial.tools.list_ports
import csv
import time
import os
from datetime import datetime

CSV_FILENAME = "C:\\Users\\User\\Documents\\DeskBuddy\\TheDeskBuddy\\sensor_log.csv"
DEVICE_NAME = "Silicon Labs CP210x USB to UART Bridge"
BAUD_RATE = 115200
SCAN_INTERVAL = 5  # seconds between scans for the device

def find_esp32_port():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if DEVICE_NAME in port.description:
            return port.device
    return None

def log_from_esp32(port):
    print(f"[INFO] Found ESP32 on port {port}")
    if os.path.exists(CSV_FILENAME):
        os.remove(CSV_FILENAME)
        print(f"[INFO] Deleted old {CSV_FILENAME}")

    try:
        ser = serial.Serial(port, BAUD_RATE, timeout=1)
        time.sleep(2)  # allow ESP32 to reset

        with open(CSV_FILENAME, mode='w', newline='') as file:
            writer = csv.writer(file)
            writer.writerow(['Real Timestamp', 'Elapsed Time (s)', 'Distance (cm)', 'Temperature (C)', 'Humidity (%)'])
            print(f"[INFO] Logging started. Writing to {CSV_FILENAME}")

            while True:
                line = ser.readline().decode('utf-8').strip()

                if line and ',' in line and not line.startswith("⚠️") and not line.startswith("🔔"):
                    try:
                        parts = line.split(',')
                        if len(parts) == 4:
                            elapsed_sec = int(parts[0])
                            distance = float(parts[1])
                            temperature = float(parts[2])
                            humidity = int(parts[3])

                            timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                            writer.writerow([timestamp, elapsed_sec, distance, temperature, humidity])
                            file.flush()
                            os.fsync(file.fileno())
                            print(f"[LOGGED] {timestamp} | {distance:.1f} cm | {temperature:.1f} °C | {humidity}%")
                    except ValueError:
                        print("[WARN] Skipped malformed line:", line)

    except serial.SerialException as e:
        print(f"[ERROR] Serial connection lost or failed: {e}")
    except KeyboardInterrupt:
        print("\n[INFO] Logging stopped by user.")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
            print("[INFO] Serial port closed.")

def wait_for_device():
    print("[INFO] Waiting for ESP32 device...")
    try:
        while True:
            port = find_esp32_port()
            if port:
                log_from_esp32(port)
                print("[INFO] Waiting again for new connection...")
            time.sleep(SCAN_INTERVAL)
    except KeyboardInterrupt:
        print("\n[INFO] Script terminated by user.")

if __name__ == "__main__":
    wait_for_device()
