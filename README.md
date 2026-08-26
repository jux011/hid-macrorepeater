# hid-macrorepeater

reads keystrokes from keyboard
plays keystrokes to computer
records keystrokes from keyboard with passthrough
replays keystrokes to computer
the quick brown fox jumps over the lazy dog

Arduino IDE
Raspberry Pi Pico Arduino core
TinyUSB device, TinyUSB host
adapted from Adafruit/DualRole/HID/hid_remapper script

target hardware:
waveshare rp2040-pizero with pio-usb
+ gamepi_lcd_hat for BCM
+ custom BCM modifications (cut pizero pin 6, solder pad 6 to pad 9)

ST7735 spi lcd controller 128p x 128p
3 buttons
5 button-joystick
