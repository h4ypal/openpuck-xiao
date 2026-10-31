// Probe for the XIAO boot crash: report MBR/SoftDevice state over USB serial, then make the same SoftDevice
// call InternalFS makes, then mount InternalFS -- with a countdown before each step so the host can attach.
#include <Arduino.h>
#include <Adafruit_TinyUSB.h>
#include <InternalFileSystem.h>
#include <nrf_sdm.h>

static void countdown(const char *what)
{
	for (int i = 5; i > 0; i--) {
		Serial.printf("%s in %d\n", what, i);
		Serial.flush();
		delay(1000);
	}
}

void setup()
{
	Serial.begin(115200);
	uint32_t rr = NRF_POWER->RESETREAS;
	NRF_POWER->RESETREAS = rr;
	while (!Serial && millis() < 15000)
		delay(10);

	const volatile uint32_t *ram = (const volatile uint32_t *)0x20000000;
	Serial.printf("RESETREAS=%08lX VTOR=%08lX\n", rr, SCB->VTOR);
	Serial.printf("RAM[0x20000000..]: %08lX %08lX %08lX %08lX\n", ram[0],
		      ram[1], ram[2], ram[3]);
	Serial.printf("CONTROL=%08lX IPSR=%08lX BASEPRI=%08lX\n", __get_CONTROL(),
		      __get_IPSR(), __get_BASEPRI());
	Serial.printf("SVCall prio=%u\n", NVIC_GetPriority(SVCall_IRQn));

	countdown("sd_softdevice_is_enabled");
	uint8_t en = 0xAA;
	uint32_t err = sd_softdevice_is_enabled(&en);
	Serial.printf("sd_softdevice_is_enabled -> err=%lu en=%u\n", err, en);

	countdown("InternalFS.begin");
	bool ok = InternalFS.begin();
	Serial.printf("InternalFS.begin -> %d\n", ok);
}

void loop()
{
	Serial.println("probe alive");
	delay(2000);
}
