# Stellar Blade Mod: Photo Mode Patches
ASI mod that patches some of the photo mode functionality in Stellar Blade.

### Features
* Modifies Screen Percentage, automatically based on certain conditions.
  * Detects when the HUD is hidden
  * Resets back to the original state when exiting photo mode
* Ability to modify various Depth of Field values for photo mode.
* Preset system.

The screen percentage variable will override the quality of your upscaler, this may not work for NVIDIA with DLSS.
There is a delay of 2 seconds when switching presets, that is because the game has issues clearing resources when swapping to a higher screen resolution, this is a work-around.

Current default keys:
* F5 = Photo mode preset (200% by default)
* F6 = Default preset (100% by default)
* F7 = Preset 1 (32% by default for low quality mode)

### Installation
Extract the archive and copy the contents to the game directory (Right-click in Steam → Manage → Browse local files).
The package includes a DLL from [Ultimate-ASI-Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader/releases).

### Linux / Steam Deck
If you're on Linux, use the following launch arguments:
```
WINEDLLOVERRIDES="dinput8=n,b" %command%
```