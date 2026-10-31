#include "status_led.h"
#include <Arduino.h>

#if defined(OPK_BOARD_MDBT50Q_CX_40)
#include <nrf_gpio.h>

// The borrowed RX variant does not map the CX-40's active-low P0.08 LED.
#define WAKE_LED_PIN NRF_GPIO_PIN_MAP(0, 8)
#undef WAKE_LED_ON
#define WAKE_LED_ON LOW
#endif

#define WAKE_LED_OFF ((WAKE_LED_ON) == HIGH ? LOW : HIGH)
#define PULSE_MS 500u // wake flash duration

static unsigned long g_pulseMs = 0;
static bool g_lit = false;

#if OPK_RGB_LED
#include "config.h"

// channel mask: bit0 = R, bit1 = G, bit2 = B
#define RGB_DARK 0
#define RGB_RED 1
#define RGB_GREEN 2
#define RGB_BLUE 4
#define RGB_WHITE 7

static uint8_t modeColor(uint8_t mode)
{
	if (modeIsPuck(mode))
		return RGB_WHITE;
	switch (mode) {
	case MODE_XBOX:
	case MODE_XBOX_OG:
		return RGB_GREEN;
	case MODE_SW_HORI:
	case MODE_SW_PRO:
		return RGB_RED;
	case MODE_PS5:
	case MODE_HIDGYRO:
	case MODE_PS5_GAME:
	case MODE_DS4_GAME:
	case MODE_PS3:
		return RGB_BLUE;
	case MODE_DINPUT:
		return RGB_RED | RGB_GREEN;
	case MODE_SINPUT:
		return RGB_RED | RGB_BLUE;
	}
	return RGB_DARK;
}

static uint8_t steadyColor()
{
	return OPK_MODE_LED ? modeColor(g_usbMode) : RGB_DARK;
}

// common-anode: a channel lights when its pin is LOW
static void rgbWrite(uint8_t mask)
{
	digitalWrite(LED_RED, (mask & RGB_RED) ? LOW : HIGH);
	digitalWrite(LED_GREEN, (mask & RGB_GREEN) ? LOW : HIGH);
	digitalWrite(LED_BLUE, (mask & RGB_BLUE) ? LOW : HIGH);
}

void ledInit()
{
	pinMode(LED_RED, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);
	pinMode(LED_BLUE, OUTPUT);
	rgbWrite(RGB_DARK);
}

void ledShowMode()
{
	if (!g_lit)
		rgbWrite(steadyColor());
}

void ledWakePulse()
{
	g_pulseMs = millis();
	g_lit = true;
	rgbWrite(steadyColor() == RGB_WHITE ? RGB_DARK : RGB_WHITE);
}

void ledTask()
{
	if (g_lit && millis() - g_pulseMs >= PULSE_MS) {
		g_lit = false;
		rgbWrite(steadyColor());
	}
}

#else

static void ledWrite(int level)
{
#if defined(OPK_BOARD_MDBT50Q_CX_40)
	nrf_gpio_pin_write(WAKE_LED_PIN, level);
#else
	digitalWrite(WAKE_LED_PIN_A, level);
	digitalWrite(WAKE_LED_PIN_B, level);
#endif
}

void ledInit()
{
#if defined(OPK_BOARD_MDBT50Q_CX_40)
	nrf_gpio_cfg_output(WAKE_LED_PIN);
#else
	pinMode(WAKE_LED_PIN_A, OUTPUT);
	pinMode(WAKE_LED_PIN_B, OUTPUT);
#endif
	ledWrite(WAKE_LED_OFF);
}

void ledShowMode()
{
}

void ledWakePulse()
{
	g_pulseMs = millis();
	g_lit = true;

	// light immediately at the remoteWakeup() call site, not on the next loop
	ledWrite(WAKE_LED_ON);
}

void ledTask()
{
	if (g_lit && millis() - g_pulseMs >= PULSE_MS) {
		g_lit = false;
		ledWrite(WAKE_LED_OFF);
	}
}

#endif
