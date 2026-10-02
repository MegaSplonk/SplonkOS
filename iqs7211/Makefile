PORT=/dev/ttyACM0

all:
	arduino-cli upload -b SparkFun:avr:promicro:cpu=16MHzatmega32U4 -vvv -p $(PORT)

check:
	arduino-cli compile -b SparkFun:avr:promicro:cpu=16MHzatmega32U4 -vvv -p $(PORT)


serial:
	picocom -b57600 $(PORT)

viewer:
	cat $(PORT) | tee /dev/tty | python ./viewer.py

