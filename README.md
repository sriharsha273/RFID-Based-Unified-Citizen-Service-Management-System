# RFID-Based-Unified-Citizen-Service-Management-System

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

https://drive.google.com/folderview?id=1wC8Qw4M1XTlmSEad3k9R3CLdwo6QXWav

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
- The system is powered ON and the project name is displayed on the 20×4 LCD.
- The LCD displays “Waiting for Card” until an RFID card is placed near the RFID reader.
- When the RFID card is detected, the RFID reader reads the card number and sends it to the LPC2148 microcontroller through serial communication at 9600 baud rate.
- The LPC2148 extracts the card number from the RFID data and checks whether the card is valid.
- If the card is invalid, the system displays “Invalid Card”, turns ON/blinks the red LED, and activates the buzzer.
- If the card is valid, the system displays the corresponding username and provides a menu of available services.
- The user selects a required service using the 4×4 matrix keypad:
- PAN Card
- ATM Card
- Voting Card
- Driving License
- Exit
- For PAN card, ATM, and voting operations, the user enters a password for authentication.
- In the ATM section, the system performs balance enquiry, withdrawal, and deposit operations using data stored in EEPROM.
- In the voting section, the system checks the user's voting status and allows voting only if the user has not already voted. The voting status is stored in EEPROM.
- In the driving license section, the system reads the current date from the RTC and compares it with the license expiry date.
- If the license is valid, “Valid License” is displayed; otherwise, “Invalid License. Please renew your license” is displayed.
- An officer RFID card can be used to reset the voting status, allowing the voting system to be reused.
- After completing any operation, the system returns to the main menu and waits for the next user.

 ## Authentication and Security
The system provides multiple levels of authentication:

- RFID card authentication
- Password authentication for selected services
- Officer card authentication for resetting the voting status

**Note:** For the first three services, the user must enter a password before performing the operation.

## Module Used

 ### LPC2148 Microcontroller

The LPC2148 acts as the main controller of the entire system. It manages RFID communication, LCD, keypad, EEPROM, UART and other peripherals.

### RFID Module

The RFID reader reads the unique card number and transfers it to the LPC2148 through serial communication.

### LCD Module

 A 20x4 LCD is used to display:

- User information
- Menus
- Card status
- Banking information
- Voting options
- Driving license status

### Keypad Module

The 4x4 matrix keypad is used for:

Menu selection
Password entry
Banking operations
Voting selection
RTC editing

### EEPROM Module

- The AT25LC512 EEPROM is used for storing information such as account balance and voting status.

 ### UART Module

- UART is used for serial communication, particularly for receiving RFID reader data.

### SPI Module

- SPI communication is used with the AT25LC512 EEPROM.

### RTC

- The on-chip RTC is used to obtain the current date and compare it with the driving license expiry date.

### LED and Buzzer

- LEDs provide visual status indications, while the buzzer provides an alert for an invalid RFID card.

 ## Applications
- Unified citizen service kiosks
- Digital identity systems
- RFID-based authentication systems
- Banking/ATM demonstrations
- Electronic voting demonstrations
- Driving license verification
- Embedded security systems
- Government service automation concepts
- Smart-card/RFID-based service platforms

## Feature Enhancement
The current project can be further enhanced with:

- Internet/cloud connectivity
- Web-based administration panel
- Mobile application integration
- Biometric authentication
- Fingerprint verification
- Face recognition
- Encrypted RFID communication
- Real-time online banking integration
- Real-time government database integration
- Online license verification
- Secure database management
- SMS/email notifications
- Advanced access-control mechanisms
- Detailed transaction logging
- Network-connected voting infrastructure

## Technologies Used
- **Microcontroller:** LPC2148
- **Programming Language:** Embedded C
- **Compiler:** Keil C
- **Programming Tool:** Flash Magic
- **Communication:** UART / SPI
- **Authentication:** RFID
- **Display:** 20x4 LCD
- **Input:** 4x4 Matrix Keypad
- **Memory:** AT25LC512 EEPROM
- **Timekeeping:** RTC

## Project Outcome
- Successfully developed an RFID-Based Unified Citizen Service Management System using the LPC2148 microcontroller.
- The system provides secure user identification and authentication using RFID cards.
- Multiple citizen services are integrated into a single platform.
- The system provides PAN card information such as user name, date of birth, and PAN number.
- Basic ATM operations such as balance enquiry, withdrawal, and deposit are implemented.
- An electronic voting facility is provided, with voting status stored in EEPROM.
- The system provides driving license information and validity checking using RTC-based date comparison.
- The project successfully integrates peripherals such as RFID reader, LCD, keypad, EEPROM, UART, SPI, LEDs, buzzer, and RTC.
- The LCD and keypad provide a simple user-friendly interface for selecting and performing different services.
- The project demonstrates practical knowledge of Embedded C programming and LPC2148 peripheral interfacing.
- The system provides a basic prototype for integrating multiple citizen services through a single RFID-based platform.

## Conclusion
- The RFID-Based Unified Citizen Service Management System provides a prototype approach for combining multiple citizen services into a single RFID-enabled embedded platform.

- By integrating the LPC2148 microcontroller, RFID reader, LCD, keypad, EEPROM, UART, SPI, LEDs, buzzer and RTC, the system demonstrates secure user identification and access to different services.

- The project also provides practical experience in Embedded C programming, microcontroller interfacing, RFID communication, SPI, UART, EEPROM, LCD and keypad interfacing.
