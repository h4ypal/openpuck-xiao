// Seeed XIAO nRF52840: route InternalFS flash writes through the NVMC instead of the SoftDevice.
//
// The Adafruit core's flash driver (InternalFileSytem/src/flash/flash_nrf5x.c) always calls
// sd_flash_page_erase()/sd_flash_write(), even while the SoftDevice is disabled. S140 v6.1.1 (Pro Micro /
// Feather) services those synchronously, but the XIAO's S140 v7.3.0 HardFaults inside the call, so the very
// first InternalFS.begin() boot-loops the board before USB comes up. OpenPuck drives the RADIO itself and never
// enables the SoftDevice, so the NVMC is always safe here. The XIAO build links with
// -Wl,--wrap=flash_nrf5x_{write,read,flush,erase} (Makefile XIAO_LINK_FLAGS), keeping the core's page cache.
#if defined(ARDUINO_XIAO_NRF52840)
#include <Arduino.h>
#include <string.h>

extern "C" {
#include "flash/flash_cache.h"

extern uint32_t __flash_arduino_start[];
}

// Adafruit UF2 bootloader start; nothing at or above it may be written
#define XIAO_BOOTLOADER_ADDR 0xF4000UL

static void nvmcWait(void)
{
	while (!NRF_NVMC->READY) {
	}
}

static bool nvmcErase(uint32_t addr)
{
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een;
	nvmcWait();
	NRF_NVMC->ERASEPAGE = addr;
	nvmcWait();
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
	nvmcWait();
	return true;
}

static uint32_t nvmcProgram(uint32_t dst, void const *src, uint32_t len)
{
	const uint8_t *s = (const uint8_t *)src;

	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
	nvmcWait();
	for (uint32_t i = 0; i + 4 <= len; i += 4) {
		uint32_t w;

		memcpy(&w, s + i, 4);
		*(volatile uint32_t *)(dst + i) = w;
		nvmcWait();
	}
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
	nvmcWait();
	return len;
}

static uint32_t nvmcRead(void *dst, uint32_t src, uint32_t len)
{
	memcpy(dst, (const void *)src, len);
	return len;
}

static bool nvmcVerify(uint32_t addr, void const *buf, uint32_t len)
{
	return memcmp((const void *)addr, buf, len) == 0;
}

static uint8_t s_cacheBuf[FLASH_CACHE_SIZE] __attribute__((aligned(4)));

static flash_cache_t s_cache = {
	nvmcErase, nvmcProgram, nvmcRead, nvmcVerify,
	FLASH_CACHE_INVALID_ADDR, s_cacheBuf,
};

extern "C" {

void __wrap_flash_nrf5x_flush(void)
{
	flash_cache_flush(&s_cache);
}

int __wrap_flash_nrf5x_write(uint32_t dst, void const *src, uint32_t len)
{
	if (dst < (uint32_t)__flash_arduino_start ||
	    dst + len > XIAO_BOOTLOADER_ADDR)
		return -1;
	return flash_cache_write(&s_cache, dst, src, len);
}

int __wrap_flash_nrf5x_read(void *dst, uint32_t src, uint32_t len)
{
	return flash_cache_read(&s_cache, dst, src, len);
}

bool __wrap_flash_nrf5x_erase(uint32_t addr)
{
	return nvmcErase(addr);
}
}
#endif
