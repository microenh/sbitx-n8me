#! /usr/bin/python
import RPi.GPIO as GPIO

GPIO.setmode(GPIO.BOARD)
GPIO.setwarnings(False)

pins_in = (7, 11, 13, 15, 19, 21, 23, 29)
pins_out = (16, 18, 22, 24, 26, 36)

for i in pins_in:
    GPIO.setup(i, GPIO.IN, pull_up_down=GPIO.PUD_UP)

for i in pins_out:
    GPIO.setup(i, GPIO.OUT)
    GPIO.output(i, GPIO.LOW)
