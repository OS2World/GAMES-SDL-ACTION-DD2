# Dodgin' Diamond 2 — ArcaOS/OS2 SDL2 Port

**Version 0.2.2b — Release 2**

A shoot-em-up arcade game originally written by Juan J. Martinez. This repository contains an ArcaOS/OS2 port based on the upstream 0.2.2 source, migrated from SDL1 to SDL2.

---

## What Changed from 0.2.2

### SDL2 Migration
- Replaced SDL1 surface/flip pipeline with SDL2 window → renderer → texture pipeline
- Virtual 320×200 back buffer scaled to the window via `SDL_RenderSetLogicalSize`
- `SDL_DisplayFormat` → `SDL_ConvertSurfaceFormat(RGB565)`
- `SDL_SetColorKey` updated to SDL2 API
- `SDL_GetKeyState` → `SDL_GetKeyboardState` with scancode wrapper

### New Features
- **Alt+Enter** toggles fullscreen (borderless desktop fullscreen)
- **Ctrl+X** exits the game immediately from anywhere
- Minimum window size 1024×768; game logic remains at native 320×200
- `.def` file added (`WINDOWAPI`) — no console window on OS/2
- User configuration stored in `$XDG_CONFIG_HOME/dd2/dd2.cfg` (falls back to `~/.config/dd2/dd2.cfg`)
- Diagnostic log written to `data/dd2.log` on startup

### Bug Fixes (Release 2)
- Config parser rewritten to use a temporary struct — a malformed or old-format config file no longer silently sets `SOUND=0` and disables audio
- Removed `SDL_RENDERER_PRESENTVSYNC` + added 1ms idle sleep to fix fullscreen choppiness and control lag
- Restored full two-player initialization from 0.2.2 (was partially commented out in the GitHub fork)
- `extern Mix_Chunk *efx[8]` corrected (was declared as size 2)
- Visual effect timing restored: `vused->ftime=250` (was 100)
- Two-player vefx loop restored: `for(i=0;i<2;i++)` (was 1)

---

## Controls

| Action | Player 1 | Player 2 |
|---|---|---|
| Move left | ← | A |
| Move right | → | D |
| Move up | ↑ | W |
| Move down | ↓ | S |
| Fire | Space | Left Ctrl |

| Key | Action |
|---|---|
| Alt+Enter | Toggle fullscreen |
| Ctrl+X | Exit immediately |
| P | Pause / unpause |
| Escape | Quit to menu |
| F12 | Save screenshot (`scnshot.bmp`) |

Joystick is supported. Button 0 = fire, Button 1 = pause.

---

## Sound and Music

Sound effects are loaded from `data/efx1.wav` … `data/efx8.wav`.

Background music uses XM tracker files `data/bgm1.xm` (normal stages) and `data/bgm2.xm` (boss). These require SDL2_mixer to be built with XM/MOD support (libmikmod or libxmp). If music does not play, SDL2_mixer on your system may lack a tracker decoder; converting the XM files to OGG and updating `soundLoad()` in `src/main.c` is the workaround.

Sound quality is controlled from the in-game configure screen (High / Med / Low / Off).

---

## Configuration File

On first run the game writes defaults. The file format is:

```
BEGIN
SOUND=3
CONTROL_1=0
CONTROL_2=0
FULL_SCREEN=0
END
```

`SOUND`: 0=off, 1=low, 2=medium, 3=high  
`CONTROL_1` / `CONTROL_2`: 0=keyboard, 1=joystick  
`FULL_SCREEN`: 0=windowed, 1=fullscreen

> **Important:** If you have an old `data/dd2.cfg` from a pre-0.2.2b build (which included a `MUSIC=` field), delete it. The parser now guards against partial-parse corruption, but a clean config is safer.

---

## Build Requirements

- ArcaOS or OS/2 with GCC 9.2 / EMX / InnoTekLIBC
- SDL2 development headers and libraries
- SDL2_mixer development headers and libraries
- GNU Make
- WLINK (OpenWatcom linker, used via EMXOMFLD)

---

## Building

```
compile.cmd
```

This sets the required EMXOMFLD environment variables and runs:

```
make -f Makefile.os2
```

Output: `dd2.exe`

The compile log is written to `compile.log`.

### EMXOMFLD Variables

The following must be set before linking (handled automatically by `compile.cmd`):

```
SET EMXOMFLD_TYPE=WLINK
SET EMXOMFLD_LINKER=wl.exe
SET EMXOMFLD_PRELINK=0
```

### Clean

```
make -f Makefile.os2 clean
```

---

## File Layout

```
dd2.exe          — game executable
dd2.def          — OS/2 module definition (WINDOWAPI, 1 MB stack)
Makefile.os2     — ArcaOS/OS2 makefile
compile.cmd      — build script
data/
  *.bmp          — graphics
  efx1–8.wav     — sound effects
  bgm1.xm        — background music (normal)
  bgm2.xm        — background music (boss)
  game.act       — enemy action script
  dd2.cfg        — user configuration (created on first run)
  dd2.log        — startup diagnostic log
src/
  main.c         — entry point, SDL2 init, game loop
  SDL_plus.c/h   — SDL2 helpers, DD2_Flip, DD2_ToggleFullscreen
  engine.c/h     — game engine
  control.c/h    — keyboard and joystick input
  menu.c/h       — menus, configure screen, high scores
  cfg.c/h        — config and score file I/O
```

---

## Credits

**Dodgin' Diamond 2** © 2003, 2004 Juan J. Martinez  
Original source: https://jjmtactics.itch.io/dodgin-diamond-2  
Licensed under the GNU General Public License v2 or later.

ArcaOS/OS2 SDL2 port based on the 0.2.2 upstream source release.
