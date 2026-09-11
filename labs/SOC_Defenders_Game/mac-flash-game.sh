LILYGO=/dev/tty.usbmodem141301;esptool --chip esp32s3 --port /dev/tty.usbmodem141301 --baud 921600 --after no-reset write-flash 0x0 ./soc_defenders.bin
