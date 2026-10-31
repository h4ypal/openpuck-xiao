// boot_trace.h -- debug-only (-DOPK_BOOT_TRACE=1) boot breadcrumbs for boards with no usable LED or USB.
//
// Each call appends one word to a spare flash page. The UF2 bootloader's CURRENT.UF2 exposes the whole flash,
// so after a crash/boot loop: double-tap RST, read 0xE6000 out of CURRENT.UF2, and the words show how far
// each boot got. The debug UF2 ships that page as 0xFF blocks so flashing it also clears the trail. Writes
// stop once the page is full, so the first boots of a loop are kept.
#pragma once
#include <Arduino.h>

#if OPK_BOOT_TRACE
#define BOOT_TRACE_PAGE 0xE6000UL

// word tags: 0xA0xxxxxx = RESETREAS at boot, 0xB00000nn = setup stage nn, 0xC0... = HardFault stacked PC
static inline void bootTraceWrite(uint32_t v)
{
	volatile uint32_t *p = (volatile uint32_t *)BOOT_TRACE_PAGE;

	for (uint32_t i = 0; i < 1024; i++) {
		if (p[i] != 0xFFFFFFFFUL)
			continue;
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen;
		while (!NRF_NVMC->READY) {
		}
		p[i] = v;
		while (!NRF_NVMC->READY) {
		}
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren;
		while (!NRF_NVMC->READY) {
		}
		return;
	}
}
#define BOOT_TRACE(stage) bootTraceWrite(0xB0000000UL | (stage))
#else
#define BOOT_TRACE(stage)
#endif
