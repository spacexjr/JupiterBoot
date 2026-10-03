#!/usr/bin/env bash
echo "========================================================"
printf '\033[38;5;223m'
echo '     ██╗██╗   ██╗██████╗ ██╗████████╗███████╗██████╗'

printf '\033[38;5;222m'
echo '     ██║██║   ██║██╔══██╗██║╚══██╔══╝██╔════╝██╔══██╗'

printf '\033[38;5;221m'
echo '     ██║██║   ██║██████╔╝██║   ██║   █████╗  ██████╔╝'

printf '\033[38;5;220m'
echo '██   ██║██║   ██║██╔═══╝ ██║   ██║   ██╔══╝  ██╔══██╗'

printf '\033[38;5;179m'
echo '╚█████╔╝╚██████╔╝██║     ██║   ██║   ███████╗██║  ██║'

printf '\033[38;5;178m'
echo ' ╚════╝  ╚═════╝ ╚═╝     ╚═╝   ╚═╝   ╚══════╝╚═╝  ╚═╝'

printf '\033[0m'
echo "========================================================"
set -euo pipefail

here="$(cd "$(dirname "$0")" && pwd)"
cd "$here"

CC="${CC:-clang}"
TARGET="${TARGET:-aarch64-linux-gnu}"
MAGISKBOOT="$here/magiskboot"
SRC="$here/src/jbm_menu.c"
ASM="$here/src/start.S"
BUILD="$here/out"
UNPACK="$BUILD/unpack"
STAGE="$BUILD/stage"
VERIFY="$BUILD/verify"
IMG_IN="${1:-$here/boot.img}"
# magiskboot runs from inside the staging dirs, so a relative IMG_IN would
# resolve against them. Anchor it to this script's directory.
case "$IMG_IN" in
  /*) ;;
  *) IMG_IN="$here/$IMG_IN" ;;
esac
IMG_OUT="$BUILD/boot-jbm.img"

JBM_VERSION="${JBM_VERSION:-1.0}"
JBM_AUTOBOOT_SEC="${JBM_AUTOBOOT_SEC:-15}"
JBM_TOTAL_TIMEOUT_SEC="${JBM_TOTAL_TIMEOUT_SEC:-240}"

say() { printf '\033[1;36m==>\033[0m %s\n' "$*"; }
die() { printf '\033[1;31m!!\033[0m %s\n' "$*" >&2; exit 1; }

command -v "$CC" >/dev/null || die "clang not found"
[ -x "$MAGISKBOOT" ] || die "magiskboot not found at $MAGISKBOOT"
[ -f "$IMG_IN" ] || die "input boot image not found: $IMG_IN"
mkdir -p "$BUILD"

say "compiling freestanding $TARGET binary"
"$CC" \
  --target="$TARGET" \
  -ffreestanding -nostdlib -static -fno-builtin -fno-stack-protector -fuse-ld=lld \
  -Os -Wall -Wextra -Wno-unused-parameter \
  -DJBM_REAL_INIT='"/init.system"' \
  "-DJBM_VERSION=\"$JBM_VERSION\"" \
  "-DJBM_AUTOBOOT_SEC=$JBM_AUTOBOOT_SEC" \
  "-DJBM_TOTAL_TIMEOUT_SEC=$JBM_TOTAL_TIMEOUT_SEC" \
  -o "$BUILD/jbm_init" "$SRC" "$ASM" \
  -Wl,-e,_start -Wl,--gc-sections
file "$BUILD/jbm_init"
if nm -u "$BUILD/jbm_init" 2>/dev/null | grep -q .; then
  nm -u "$BUILD/jbm_init" | head
  die "binary has undefined symbols"
fi

say "building host UI preview"
"$CC" -DJBM_HOST -O2 -Wall -Wextra -Wno-unused-parameter -o "$BUILD/jbm_host" "$SRC"
"$BUILD/jbm_host" "$BUILD/ui-preview.ppm" 4000 0
"$BUILD/jbm_host" "$BUILD/ui-preview-intro.ppm" 120 0
"$BUILD/jbm_host" "$BUILD/ui-preview-sel.ppm" 4000 1
"$BUILD/jbm_host" "$BUILD/ui-booting.ppm" 600 2
"$BUILD/jbm_host" "$BUILD/ui-action.ppm" 600 3
MAGICK_BIN=$(command -v magick >/dev/null 2>&1 && echo magick || echo convert)
for p in ui-preview ui-preview-intro ui-preview-sel ui-booting ui-action; do
  [ -f "$BUILD/$p.ppm" ] && "$MAGICK_BIN" "$BUILD/$p.ppm" "$BUILD/$p.png"
done

say "unpacking $(basename "$IMG_IN")"
rm -rf "$UNPACK" "$STAGE" "$VERIFY"
mkdir -p "$UNPACK"
( cd "$UNPACK" && "$MAGISKBOOT" unpack -h "$IMG_IN" >/dev/null )
[ -f "$UNPACK/ramdisk.cpio" ] || die "boot image has no ramdisk.cpio"

say "preparing staging copy"
cp -a "$UNPACK" "$STAGE"
cp -f "$STAGE/ramdisk.cpio" "$BUILD/ramdisk-orig.cpio"

# The menu's diagnostics go through /dev/kmsg, but the default kernel ring
# wraps within ~15 s of a busy boot and adb is only up at ~70 s, so the lines
# are gone before anyone can read them. Growing the ring keeps the whole boot.
if [ -f "$STAGE/header" ] && grep -q '^cmdline=' "$STAGE/header"; then
  grep -q 'log_buf_len=' "$STAGE/header" ||
    sed -i '/^cmdline=/ s/$/ log_buf_len=4M/' "$STAGE/header"
fi

say "swapping ramdisk /init for the boot manager"
chmod 0750 "$BUILD/jbm_init"
"$MAGISKBOOT" cpio "$STAGE/ramdisk.cpio" \
  "mv init init.system" \
  "add 0750 init $BUILD/jbm_init" || die "failed to install /init"
"$MAGISKBOOT" cpio "$STAGE/ramdisk.cpio" ls init init.system >/dev/null 2>&1 ||
  die "cpio missing init or init.system after the swap"
"$MAGISKBOOT" cpio "$STAGE/ramdisk.cpio" test >/dev/null 2>&1 || die "cpio status is unsupported"

say "repacking boot image"
( cd "$STAGE" && "$MAGISKBOOT" repack "$IMG_IN" "$IMG_OUT" >/dev/null )
[ -f "$IMG_OUT" ] || die "repack produced no output"

say "verifying output"
mkdir -p "$VERIFY"
( cd "$VERIFY" && "$MAGISKBOOT" unpack -h "$IMG_OUT" >/dev/null )
[ -f "$VERIFY/header" ] && grep -q '^cmdline=.*log_buf_len=4M' "$VERIFY/header" ||
  die "cmdline of the output does not carry log_buf_len=4M"
cmp -s "$VERIFY/ramdisk.cpio" "$STAGE/ramdisk.cpio" ||
  die "ramdisk read back from the output does not match the staged one"
( cd "$VERIFY" && "$MAGISKBOOT" cpio ramdisk.cpio test >/dev/null 2>&1 ) ||
  die "ramdisk in the output is broken"
rm -f "$VERIFY/init"
# magiskboot's extract returns 1 even on success, so the file is the real check.
( cd "$VERIFY" && "$MAGISKBOOT" cpio ramdisk.cpio extract init >/dev/null 2>&1 ) || true
cmp -s "$VERIFY/init" "$BUILD/jbm_init" ||
  die "/init in the output differs from the freshly built binary"
[ "$(stat -c %s "$IMG_OUT")" = "$(stat -c %s "$IMG_IN")" ] ||
  die "output image size changed: $(stat -c %s "$IMG_IN") -> $(stat -c %s "$IMG_OUT")"

printf '\ninput  : %s (%s bytes)\n' "$IMG_IN" "$(stat -c %s "$IMG_IN")"
printf 'output : %s (%s bytes)\n' "$IMG_OUT" "$(stat -c %s "$IMG_OUT")"
printf 'menu   : %s (%s bytes)\n' "$BUILD/jbm_init" "$(stat -c %s "$BUILD/jbm_init")"
