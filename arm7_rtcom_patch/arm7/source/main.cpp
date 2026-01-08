#include <maxmod7.h>
#include <nds.h>
#include <calico.h>

#include "rtcom.h"

void VblankHandler(void) { Update_RTCom(); }

volatile bool exitflag = false;

__attribute__((target("arm"))) int main() {
	lcdSetIrqMask(DISPSTAT_IE_ALL, DISPSTAT_IE_VBLANK);
	irqEnable(IRQ_VBLANK);
    irqSet(IRQ_VBLANK, VblankHandler);

    // Keep the ARM7 mostly idle
    while (!exitflag) {
        if (0 == (REG_KEYINPUT & (KEY_SELECT | KEY_START | KEY_L | KEY_R))) {
            exitflag = true;
        }
		threadWaitForVBlank();
    }
    return 0;
}
