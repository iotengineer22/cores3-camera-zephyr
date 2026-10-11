import serial
import time
import sys

port = 'COM9'
baud = 115200

print(f"Opening {port} at {baud} bps...")
try:
    ser = serial.Serial(port, baud, timeout=0.5)
except Exception as e:
    print(f"Failed to open port: {e}")
    sys.exit(1)

# Reset ESP32-S3 via DTR/RTS
print("Resetting ESP32-S3...")
ser.setDTR(False)
ser.setRTS(True)
time.sleep(0.1)
ser.setRTS(False)
time.sleep(0.2)

print("Reading logs for 5 seconds...\n---------------------------------------------")
start_time = time.time()
lines = []
while time.time() - start_time < 5.0:
    line = ser.readline()
    if line:
        decoded = line.decode('utf-8', errors='replace').rstrip()
        print(decoded)
        lines.append(decoded)

ser.close()
print("---------------------------------------------\nFinished reading.")
