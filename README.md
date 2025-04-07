# HGSS/Pokewalker communication using 3DS built-in infrared transceiver

> **This project is still EXPERIMENTAL, please consider that it might not work consistently and on all consoles.**

This is a patch for HG/SS games that enables communications with the Pokewalker without needing a IR capable cartridge, as it uses the 3DS built-in IR transceiver.
Thanks to **RTCom**, the ARM7 processor is able to communicate with the ARM11, which has access to the IR transceiver registers.

If you are interested in another project related to Pokewalker hacking, check [**pwalkerHax**](https://github.com/francesco265/pwalkerHax).
You can find a demo of my two projects in action [Here](https://youtu.be/f6f8RSxqG20) (**pwalkerHax** + **RtcPwalker**).

## How to use

1. Patch the **TWL_FIRM** using [TWPatch](https://www.gamebrew.org/wiki/TWPatch_3DS), make sure that **RTCom** is enabled before patching. You can use this [guide](https://wiki.ds-homebrew.com/twilightmenu/playing-in-widescreen) as a reference on how to patch the firmware and how to load it when using **TWiLight Menu++**.
    - **NOTE:** If you decide to patch the TWL_FIRM enabling RTCom **only** you can just leave the patched `TwlBg.cxi` in the `sd:/luma/sysmodules/`, in this way every time you boot a DS game, the patched TWL_FIRM will be loaded. Just remember to enable `external FIRMs and modules` in the Luma3DS configuration.
2. The game patch is implemented as an Action Replay code: just add the Action Replay code relative to your game version to your **usrcheat.dat** file or directly use the **usrcheat.dat** provided (place it in `_nds/TWiLightMenu/extras/`). All files can be found in the [**releases**](https://github.com/francesco265/RtcPwalker/releases/latest) GitHub section.
3. Enable the patch in the cheat menu of **TWiLight Menu++**.

## Disclaimers

- Communication with the Pokewalker is tricky, as the 3DS needs to follow the strict timings of the Pokewalker. RTCom adds a big overhead to the communication, thus the communication is **SLOW and UNSTABLE**, but it works :wink:.
Simply try again if it fails, especially when sending the pokemon to the Pokewalker, it should work after a few tries. If it still doesn't work, try to return to the Pokewalker selection menu and try again.
- Make sure to point the Pokewalker towards the 3DS IR transceiver, which is the black spot on the back side of the console.
- **If the game doesn't boot** while the patch is enabled, it means that you don't have **RTCom** installed: make sure that TWiLight Menu is loading the patched TWL_FIRM.
- This patch was only tested on an **Old3DS**, as it is the only console I have access to. It should work on a **New3DS** as well, but I can't guarantee it.
- I haven't tested if the game works while this patch is enabled, so I would recommend to disable it if you want to play the game normally, even though there shouldn't be any problem.

## Credits

This project wouldn't have been possible without the contributions of the community, in particular:
- [**TWPatch/RTCom**](https://gbatemp.net/threads/twpatcher-ds-i-mode-screen-filters-and-patches.542694/) by _Sono_ and _Gericom_.
- [**Circle Pad patch for SM64DS**](https://gbatemp.net/threads/circle-pad-patches-for-super-mario-64-ds-and-other-games-in-twilightmenu-with-twpatcher-and-rtcom.623267/) by _Shoco_ -- I borrowed many parts of his code.
- [**Pokewalker hacking**](https://dmitry.gr/?r=05.Projects&proj=28.%20pokewalker) by _Dmitry_ -- Thanks to his excellent work I was able to understand the communication protocol between the Pokewalker and the game, it was very useful during debugging.
- [**libn3ds**](https://github.com/profi200/libn3ds) by _profi200_ -- I used his implementation of the i2c protocol to communicate with the IR transceiver.

---
This project required many hours of reverse engineering and painful debugging, if you appreciate my works and want to support me, you can offer me a coffee :coffee::heart:.

- **BTC**: `bc1qvvjndu7mqe9l2ze4sm0eqzkq839m0w6ldms8ex`
- **PayPal**: [![](https://www.paypalobjects.com/en_US/i/btn/btn_donate_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=AAZDH3SM7T9P6)
