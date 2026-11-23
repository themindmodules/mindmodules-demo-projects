// Minimal Arduino API stubs so the sketch can be syntax-checked without the Arduino toolchain.
// Only declarations: nothing here is meant to run.
#ifndef ARDUINO_STUB_H
#define ARDUINO_STUB_H
#include <stdint.h>

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define MSBFIRST 1
#define LSBFIRST 0
#define F(s) (s)

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
void shiftOut(uint8_t dataPin, uint8_t clockPin, uint8_t bitOrder, uint8_t val);
void delayMicroseconds(unsigned int us);
unsigned long millis(void);

struct SerialStub {
  void begin(unsigned long baud);
  void print(const char *s);
  void print(char c);
  void print(int v);
  void println(void);
};
extern SerialStub Serial;

#endif
