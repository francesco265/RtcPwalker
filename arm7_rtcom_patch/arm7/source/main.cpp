#include <dswifi7.h>
#include <maxmod7.h>
#include <nds.h>

#include "rtcom.h"

void VblankHandler(void) { Update_RTCom(); }

volatile bool exitflag = false;

__attribute__((target("arm"))) int main() {
    irqInit();
    irqSet(IRQ_VBLANK, VblankHandler);
    irqEnable(IRQ_VBLANK);

    // Keep the ARM7 mostly idle
    while (!exitflag) {
        if (0 == (REG_KEYINPUT & (KEY_SELECT | KEY_START | KEY_L | KEY_R))) {
            exitflag = true;
        }
        swiWaitForVBlank();
    }
    return 0;
}
