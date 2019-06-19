### IrRemote-attiny-i2c

- I2C controlled IrRemote implemented in Attiny85 and Attiny1634
- Attiny85/1634 uses Arduino [TinyCore](https://github.com/SpenceKonde/ATTinyCore)
- IrRemote library uses [this](https://github.com/l740416/Arduino-IRremote)
- The default I2C address: 0x05
- Due to program space in Attiny85 (8KB), you will need to disable some supported IR protocols in IrRemote library

### Pins connection

|      Pin      |    Attiny85   |   Attiny1634  |
| ------------- | ------------- | ------------- |
|      SDA      |      PB0      |      PB1      |
|      SCL      |      PB2      |      PC1      |
|    Ir Send    |      PB1      |      PB3      |
|    Ir Recv    |      PB3      |      PA0      |
|     LED1      |      PB4      |      PC0      |
|     LED2      |      PB4      |      PC2      |

- SDA/SCL/IrSend pins are fixed
- LED1: status LED
  - Fast blink twice: send code
  - Slow blink twice: send raw code
  - Fast blink once: stop recording incoming code
  - Slow blink once: received code
- LED2: blink when receiving IR signals and processing
