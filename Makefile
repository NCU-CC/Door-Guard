ARDUINO_DIR = /usr/local/share/arduino
ARDMK_DIR = /usr/local/share/arduino-mk
AVR_TOOLS_DIR = /usr/local/share/arduino/hardware/tools/avr
AVRDUDE_CONF = /usr/local/share/arduino/hardware/tools/avr/etc/avrdude.conf
ARDUINO_CORE_PATH = /usr/local/share/arduino/hardware/arduino/avr/cores/arduino
BOARDS_TXT = /usr/local/share/arduino/hardware/arduino/avr/boards.txt
ARDUINO_VAR_PATH = /usr/local/share/arduino/hardware/arduino/avr/variants
BOOTLOADER_PARENT = /usr/local/share/arduino/hardware/arduino/avr/bootloaders
USER_LIB_PATH = /home/ywjameslin/Arduino/libraries

ARCHITECTURE  = avr
BOARD_TAG     = mega
BOARD_SUB     = atmega2560
ARDUINO_PORT = /dev/ttyACM0
ARDUINO_LIBS = Wire SPI Ethernet LiquidCrystal_I2C Keypad Keypad_I2C NDEF PN532_HSU PN532
ARDUINO_VERSION = 10801

include /usr/local/share/arduino-mk/Arduino.mk
