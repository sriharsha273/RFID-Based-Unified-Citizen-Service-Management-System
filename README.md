## RFID-Based-Unified-Citizen-Service-Management-System

## Objective

To design and develop an RFID-Based Unified Citizen Service Management System
that enables secure user authentication and provides multiple citizen services such as digital
identity verification, banking operations, electronic voting, and driving license validation
through a single RFID-based platform. 

## Block Diagram
<p align="center">
  <img src="Block Diagram.jpg" alt="Block Diagram" width="500">
</p>


## Project images and videos



## Features
- RFID-based user authentication
- Valid and invalid card detection
- Username identification
- Password-based security
- PAN card information display
- ATM/banking operations
- Balance enquiry
- Withdrawal facility
- Deposit facility
- Electronic voting
- Voting-status storage
- Driving license information display
- Driving license validity verification
- Officer card support for resetting voting status
- LCD-based menu system
- Keypad-based user interaction
- LED and buzzer indications
- RTC-based date verification
- EEPROM data storage

## Hardware Requirements
- LPC 2148
- RFID Reader
- RFID cards
- 20x4 LCD
- 4x4 Matrix keypad
- AT25LC512
- LED’S
- Buzzer
- USB-UART Converter
  
## Software Requirements
- EMBEDDED C 
- KEIL uVision 
- Flash Magic

## Working Principle
1. The system is powered ON and the project name is displayed on the 20×4 LCD.
2 .The LCD displays “Waiting for Card” until an RFID card is placed near the RFID reader.
When the RFID card is detected, the RFID reader reads the card number and sends it to the LPC2148 microcontroller through serial communication at 9600 baud rate.
The LPC2148 extracts the card number from the RFID data and checks whether the card is valid.
If the card is invalid, the system displays “Invalid Card”, turns ON/blinks the red LED, and activates the buzzer.
If the card is valid, the system displays the corresponding username and provides a menu of available services.
The user selects a required service using the 4×4 matrix keypad:
PAN Card
ATM Card
Voting Card
Driving License
Exit
For PAN card, ATM, and voting operations, the user enters a password for authentication.
In the ATM section, the system performs balance enquiry, withdrawal, and deposit operations using data stored in EEPROM.
In the voting section, the system checks the user's voting status and allows voting only if the user has not already voted. The voting status is stored in EEPROM.
In the driving license section, the system reads the current date from the RTC and compares it with the license expiry date.
If the license is valid, “Valid License” is displayed; otherwise, “Invalid License. Please renew your license” is displayed.
An officer RFID card can be used to reset the voting status, allowing the voting system to be reused.
After completing any operation, the system returns to the main menu and waits for the next user.
