#!/bin/bash
gpioset gpiochip0 17=0
sleep 0.1
gpioset gpiochip0 17=1
sleep 0.1
