import serial
import time

# Replace 'COMx' with your actual COM port (e.g., COM5 on Windows, /dev/ttyUSB0 on Linux)
bluetooth_port = 'COM7'
baud_rate = 9600

try:
    bt = serial.Serial(bluetooth_port, baud_rate)
    print("Connected to Bluetooth device.")
    time.sleep(2)  # Wait for connection to stabilize

    while True:
        if bt.in_waiting:
            data = bt.readline().decode('utf-8').strip()
            print(data)

except serial.SerialException as e:
    print(f"Serial error: {e}")
except KeyboardInterrupt:
    print("Exiting program.")
finally:
    if 'bt' in locals() and bt.is_open:
        bt.close()
