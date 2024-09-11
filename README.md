# Pokewalker communication using 3DS built-in infrared transceiver

This is a patch for HG/SS games that allows you to use the Pokewalker without needing a IR capable cartridge. It uses the 3DS built-in IR transceiver and **Rtcom**.

The patch is implemented as an Action Replay code. **FINIRE QUA**

## Disclaimer

This patch is still a **beta** and may not work in a stable way, it was only tested on an Old 3DS, i don't know if it works on any other device.

In order for this patch to work you need to patch the **TWL_FIRM** using [TWPatch](https://www.gamebrew.org/wiki/TWPatch_3DS), make sure that **rtcom** is enabled before patching.

## Working features

For now, the following features have been tested and are working:

- [x] Reset the Pokewalker
- [x] Syncing the Pokewalker with the game and go for a stroll
- [x] Receive gifts
- [ ] Return from a stroll
- [ ] Go for a stroll
