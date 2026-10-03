# JupiterBoot

A graphical, touch-driven boot menu injected into the `boot` ramdisk of a
Samsung Galaxy A22 4G (`a22` / SM-A225M, MediaTek MT6768, Android 15 GSI).

`/init` is replaced by a freestanding aarch64 binary; the original Android
first-stage init is preserved as `/init.system` and is what the menu eventually
executes. The repository never flashes anything: the build only produces
`out/boot-jbm.img`, which must be reviewed and flashed manually.

## Boot flow

1. The bootloader loads `boot.img` and the kernel starts `/init` (the menu).
2. The menu mounts `proc`/`sys`, opens `/dev/fb0` and the touchscreen, and draws
   the UI.
3. After the auto-start countdown (default 15 s) it chains to `/init.system`,
   which continues the normal second-stage boot.
4. Selecting **System** switches to the booting screen, waits 2 s, then chains
   to `/init.system`.
5. Every other mode switches to the same action screen (naming the mode and what
   it is about to do) for 1.5 s before the action runs, so the tap always has
   visible feedback.
6. If the framebuffer is unavailable, the menu chains to `/init.system`
   immediately without drawing anything.

## Menu actions

Touch model: one tap runs the row under the finger (press highlights it,
release executes it).

| Action    | Behaviour                                                |
|-----------|----------------------------------------------------------|
| System    | Booting screen for 2 s, then run `/init.system`          |
| Recovery  | Reboot with reason `recovery` (no flash write)           |
| Fastboot  | Reboot to bootloader                                     |
| Download  | Reboot with reason `download`                            |
| Reboot    | Restart the device                                       |
| Power off | Shut the device down                                     |

Every row goes through the same action screen first, so there is always visible
feedback before the action runs.

Any touch cancels the countdown so the device waits for a decision; the bar is
replaced by a `MODE SELECTED - TAP TO RUN` hint.

## Framebuffer note

mtkfb only latches a frame when the layer's mode is re-applied
(`FBIOPUT_VSCREENINFO` with `FB_ACTIVATE_FORCE`), so a plain `memcpy` into the
scanout buffer can stay invisible. The menu therefore re-applies the mode after
every screen change (`gfx_reapply()`) and once a second as a heartbeat, on top
of the two full blank/unblank kicks it does at startup. Without this the panel
keeps showing the first menu frame forever.

## Safety model

- **Recovery** does not write to flash. It only logs the intent, syncs, and asks
  the kernel to reboot with the reason `recovery`
  (`reboot(LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART2,
  "recovery")`). Whether the bootloader honours that reason is up to the
  platform and is not guaranteed.
- **Download** likewise only reboots with the reason `download`; there is no
  userspace flash write.
- No menu action backs up, copies, or overwrites any partition. `boot` is left
  untouched by the menu.

## Restoring `boot` (legacy)

Earlier builds could back up `boot` to `/cache/jbmenu/boot.bak` before a
recovery swap. That code has been removed and no backup is created anymore.
If you still have a backup from an old build, it is a raw copy of the whole
32 MiB `boot` partition and can be restored from a root shell or recovery:

```sh
dd if=/cache/jbmenu/boot.bak of=/dev/block/by-name/boot
sync
```

Without a backup, reflash the stock `boot` image with Odin.

## Build

Requirements:

- Linux host with `clang` capable of `--target=aarch64-linux-gnu` (uses `lld`).
- `nm`, `file`, `stat`.
- `magiskboot` next to this README (already present).
- The original `boot.img` in the repository root.
- Python 3 + Pillow only if regenerating the font (add Inkscape for the
  planet art).

```sh
./build.sh            # uses ./boot.img
./build.sh path/to/boot.img
```

The script:

1. Builds the host UI previews from the real drawing path:
   `out/ui-preview.png` (menu, System selected, countdown running),
   `out/ui-preview-intro.png` (mid intro animation),
   `out/ui-preview-sel.png` (Recovery selected, no countdown),
   `out/ui-booting.png` (the "Booting System" screen) and
   `out/ui-action.png` (the same screen for a non-System mode).
2. Compiles the freestanding aarch64 binary to `out/jbm_init` and rejects any
   undefined symbol.
3. Unpacks `boot.img`, renames `init` to `init.system`, adds the menu as `init`,
   and repacks to `out/boot-jbm.img`.
4. Re-reads the output and verifies the ramdisk, the extracted `/init`, and that
   the image size is unchanged.

Constants such as the countdown and version can be overridden:

```sh
JBM_AUTOBOOT_SEC=10 JBM_VERSION=1.1 ./build.sh
```

## Regenerating fonts

```sh
python3 tools/gen_font.py     # rewrites src/jbm_font.h, needs Pillow
```

The font is ASCII only (32..126); middle dots in the UI are drawn as discs.

## Regenerating the planet art

The planet slices are rendered from the layout mocks `jbtest.html` (menu) and
`jbbootingtest.html` (boot screen), which are the source of truth for position
and styling:

```sh
python3 tools/gen_planet.py   # needs Inkscape + Pillow
```

It rewrites `src/jbm_planet_menu.h` and `src/jbm_planet_boot.h`.

## Layout

```
src/jbm_menu.c        menu, UI, framebuffer, touch and reboot
src/start.S           aarch64 entry point that normalises argc/argv/envp
src/jbm_font.h        generated 8-bit alpha font atlases (committed)
src/jbm_planet_*.h    generated planet slices (committed)
tools/gen_font.py
tools/gen_planet.py
jbtest.html           menu mock (layout source of truth)
jbbootingtest.html    boot-screen mock (layout source of truth)
build.sh              build + inject + verify, never flashes
out/                  build artifacts (jbm_init, boot-jbm.img, previews, staging)
```

## Untested
- Magisk 

---
Thank you
