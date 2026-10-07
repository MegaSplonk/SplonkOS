#pragma once

#define DEC 10

class Serial_ {
  public:
	void begin(long int);
	bool available(void);

	usize write(const uint8_t *, usize);
	usize write(uint8_t);

	usize print(const char[]);
	usize print(char);
	usize print(unsigned char, int = DEC);
	usize print(int, int = DEC);
	usize print(unsigned int, int = DEC);
	usize print(long, int = DEC);
	usize print(unsigned long, int = DEC);
	usize print(double, int = 2);
	usize println(const char[]);
	usize println(char);
	usize println(unsigned char, int = DEC);
	usize println(int, int = DEC);
	usize println(unsigned int, int = DEC);
	usize println(long, int = DEC);
	usize println(unsigned long, int = DEC);
	usize println(double, int = 2);
	usize println(void);

	uint8_t read(void);

	operator bool(void);

  private:
	usize printNumber(unsigned long, uint8_t);
	usize printFloat(double, uint8_t);
};

extern Serial_ Serial;
