cmake_minimum_required( VERSION 2.8 )

set( AVR_MCU attiny13a )

include( avr.cmake )

project( RGB-Beacon )

add_avr_executable( RGB-Beacon RGB-Beacon/main.cpp )