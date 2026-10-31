// Board sanity check for the vendored XIAO platform: if this cycles red -> green -> blue and shows up as a
// USB serial port printing "xiao_blink alive", the platform, link address and bootloader hand-off are fine.
#include <Arduino.h>

void setup()
{
	pinMode(LED_RED, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_BLUE, OUTPUT);
	Serial.begin(115200);
}

void loop()
{
	static const uint8_t pins[] = { LED_RED, LED_GREEN, LED_BLUE };
	static uint8_t i = 0;

	for (uint8_t p : pins)
		digitalWrite(p, HIGH);
	digitalWrite(pins[i], LOW);
	i = (i + 1) % 3;
	Serial.println("xiao_blink alive");
	delay(400);
}
