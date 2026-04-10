# Flashing the Firmware

1. Ensure esptool.py is installed. If it is not, install it with 
	- `pip install esptool`
2. Hold down the button on the back of the dongle and plug it in.
3. Verify the port used by the dongle. _(typically /dev/ttyACM0)_
    - `ls /dev/ttyACM* /dev/ttyUSB*`
4. Run `esptool --chip esp32s3 --port /dev/ttyACM0 write-flash 0x0 challenge.bin`
5. If necessary, unplug the device and plug it back in.
