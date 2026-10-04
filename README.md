# STM32L476 Smart Blind Stick

STM32 HAL reference implementation of the Smart Blind Stick described in the supplied project document. It combines three HC-SR04 distance sensors, an IR input, buzzer and vibration motor alerts, a NEO-6M GPS receiver, an HC-05 Bluetooth link, and a long-press SOS button.

## Source provenance

The supplied document contains screenshots of code, not the original CubeMX project or complete source files. The following behaviors are visible in those screenshots and are transcribed into `Core/Src/main.c`:

- Front, left, and right ultrasonic distance reads with 15 ms spacing; readings above 0.1 cm and below an obstacle threshold trigger detection.
- IR detection on a high-to-low input transition.
- Buzzer and vibration outputs activated by an obstacle or IR event and switched off after a timed interval.
- A long SOS button press after more than 3000 ms; send SOS with coordinates if available, otherwise report GPS searching.
- `$GPGGA` parsing, NMEA degree/minute conversion, and a Bluetooth `Track: %f, %f` coordinate message.

The screenshot does not show all surrounding declarations, exact GPIO assignments, UART initialization, ultrasonic driver, GPS receive transport, or the alert duration constants. Those portions are reconstructed here for a coherent STM32L476 HAL example. The ~70 cm threshold comes from the document description; validate it against your actual hardware and source. No reconstructed pin or timing value should be represented as an original project setting.

## Project layout

```text
Smart-Blind-Stick/
├── Core/
│   ├── Inc/
│   │   ├── gps.h
│   │   ├── hcsr04.h
│   │   └── main.h
│   └── Src/
│       ├── gps.c
│       ├── hcsr04.c
│       └── main.c
└── README.md
```

## Pin/configuration map

All pin assignments below are reconstructed examples. Change `Core/Inc/main.h` and the UART MSP setup in `Core/Src/main.c` to match your board.

| Function | STM32L476 example pin | Direction / notes |
|---|---|---|
| Front HC-SR04 TRIG / ECHO | PB0 / PC0 | TRIG output; ECHO input, 3.3 V max |
| Left HC-SR04 TRIG / ECHO | PB1 / PC1 | TRIG output; ECHO input, 3.3 V max |
| Right HC-SR04 TRIG / ECHO | PB2 / PC2 | TRIG output; ECHO input, 3.3 V max |
| IR sensor digital output | PC3 | Input with pull-up; code triggers on falling edge |
| Buzzer driver input | PB10 | Output; use a transistor/driver as needed |
| Vibration motor driver input | PB11 | Output; use a transistor/MOSFET and flyback protection as needed |
| SOS push button | PC13 | Active low, internal pull-up |
| NEO-6M UART | USART1 PA9 TX / PA10 RX | 9600 baud, 8-N-1; STM32 TX to GPS RX and STM32 RX to GPS TX |
| HC-05 UART | USART2 PA2 TX / PA3 RX | 9600 baud, 8-N-1; cross TX/RX |

### Electrical notes

STM32L476 GPIO is not 5 V tolerant on every pin and should not be assumed to accept a 5 V HC-SR04 echo. Use a resistor divider or level shifter on each echo line. Power motors and buzzers through suitable driver stages, not directly from MCU pins. Confirm common ground and module logic voltage before connecting power.

## Build and use

1. Create an STM32CubeIDE project for the exact STM32L476 part on your board, using the STM32L4 HAL package.
2. Add these `Core/Inc` and `Core/Src` files to the generated project, keeping the generated startup code, HAL drivers, and linker script.
3. Ensure the project uses the STM32L4 HAL and enables the USART1 interrupt handler from `main.c`. This example initializes GPIO/UART and the clock directly; do not also compile duplicate CubeMX-generated `main.c`, `SystemClock_Config`, `HAL_UART_MspInit`, or `USART1_IRQHandler` definitions.
4. Change the reconstructed pin map and UART alternate-function pins if your board differs.
5. Flash and test each subsystem with the stick safely supported before relying on it.

The code uses the Cortex-M4 DWT cycle counter to time HC-SR04 echo pulses. The GPS interrupt collects NMEA lines and processes valid GGA fixes in the main loop. Bluetooth messages are sent as readable text with CR/LF endings.

## Limitations

- This is a reconstructed reference project, not the original firmware and not a medical or safety-certified mobility aid.
- IR behavior depends on the sensor's output polarity and mounting; confirm a falling edge corresponds to the intended ground/step event.
- `0.0 cm` represents a timeout or invalid ultrasonic reading and is ignored by the recovered threshold condition.
- The SOS message is retransmitted if the user releases and presses the button again. Add application-specific latching or acknowledgement if needed.
- GPS coordinates are only sent after a valid GGA fix. Indoor reception may not be available.

## License

Choose a license before publishing if you want others to reuse or modify this project. This repository intentionally does not claim a license on your behalf.
