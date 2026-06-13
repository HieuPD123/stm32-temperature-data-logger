from datetime import datetime
import serial

ser = serial.Serial(
    port='COM3',      # sửa thành cổng COM của bạn
    baudrate=115200,
    timeout=1
)

now = datetime.now()

time_str = now.strftime("%d,%m,%Y,%H,%M,%S\n")

ser.write(time_str.encode())

print("Sent:", time_str)

ser.close()