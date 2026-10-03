#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdarg.h>
#include <stdint.h>

#include "jbm_font.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int64_t i64;
typedef int32_t i32;

#define NULL ((void *)0)
#define AT_FDCWD (-100)

struct jbm_timespec {
    i64 tv_sec;
    long tv_nsec;
};

#ifndef JBM_REAL_INIT
#define JBM_REAL_INIT "/init.system"
#endif
#ifndef JBM_AUTOBOOT_SEC
#define JBM_AUTOBOOT_SEC 15
#endif
#ifndef JBM_VERSION
#define JBM_VERSION "1.0"
#endif
#ifndef JBM_TOTAL_TIMEOUT_SEC
#define JBM_TOTAL_TIMEOUT_SEC 240
#endif

/* Duration of the entry slide-in animation, in ms. */
#define JBM_INTRO_MS 480

/* How long the "starting X" screen stays up before the action is carried out,
 * in ms. Long enough for the spinner to be seen, short enough not to be in the
 * way. */
#define JBM_RUN_MS 1400

/* errno of the last failed jbm_mmap(), 0 otherwise (diagnostics only). */
static int jbm_mmap_err = 0;

#define LINUX_REBOOT_MAGIC1 0xfee1dead
#define LINUX_REBOOT_MAGIC2 0x28121969
#define LINUX_REBOOT_CMD_RESTART 0x01234567
#define LINUX_REBOOT_CMD_POWER_OFF 0x4321FEDC
#define LINUX_REBOOT_CMD_RESTART2 0xA1B2C3D4

#define JB_RB_RESTART 0
#define JB_RB_BOOTLOADER 1
#define JB_RB_POWER_OFF 2
#define JB_RB_DOWNLOAD 3
#define JB_RB_RECOVERY 4

/* ------------------------------------------------------------------ */
/* syscalls                                                            */
/* ------------------------------------------------------------------ */

#ifdef JBM_HOST

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <linux/fb.h>
#include <linux/input.h>

static i64 hs_read(int fd, void *b, u32 n) { return (i64)read(fd, b, n); }
static i64 hs_write(int fd, const void *b, u32 n) { return (i64)write(fd, b, n); }
static i64 hs_open(const char *p, int fl, int md) { return open(p, fl, md); }
static int hs_close(int fd) { return close(fd); }
static int hs_ioctl(int fd, unsigned long r, void *a) { return ioctl(fd, r, a); }
static void *hs_mmap(u32 len, int prot, int flags, int fd, long off) {
    (void)prot;
    if (flags & MAP_ANONYMOUS) return calloc(1, len ? len : 1);
    (void)fd;
    (void)off;
    return calloc(1, len ? len : 1);
}
static i64 hs_poll(void *fds, u32 n, int t) { return (i64)poll((void *)fds, (nfds_t)n, t); }
static i64 hs_getdents(int fd, void *b, u32 n) { return (i64)syscall(SYS_getdents64, fd, b, n); }
static i64 hs_mkdir(const char *p, int m) { return (i64)mkdir(p, (mode_t)m); }
static i64 hs_mknod(const char *p, int m, int d) { return (i64)mknod(p, (mode_t)m, (dev_t)d); }
static int hs_mount(const char *s, const char *t, const char *ty, u64 fl, const void *d) {
    return mount(s, t, ty, fl, d) == 0 ? 0 : -1;
}
static int hs_umount(const char *t) { return umount(t) == 0 ? 0 : -1; }
static int hs_fsync(int fd) { return fsync(fd); }
static i64 hs_now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (i64)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
static void hs_sleep_ms(u32 ms) { usleep(ms * 1000); }
static i64 hs_reboot_cmd(int cmd, int sub) {
    static const char *const names[] = {"restart", "bootloader", "power off", "download", "recovery"};
    if (cmd >= 0 && cmd < 5) fprintf(stderr, "[jbm:host] would reboot: %s\n", names[cmd]);
    else fprintf(stderr, "[jbm:host] would reboot: unknown (%d)\n", cmd);
    (void)sub;
    return 0;
}

#define jbm_open hs_open
#define jbm_read hs_read
#define jbm_write hs_write
#define jbm_close hs_close
#define jbm_ioctl hs_ioctl
#define jbm_mmap hs_mmap
#define jbm_pollfd_ts hs_poll
#define jbm_getdents hs_getdents
#define jbm_mkdir hs_mkdir
#define jbm_mknod hs_mknod
#define jbm_mount hs_mount
#define jbm_umount hs_umount
#define jbm_fsync hs_fsync
#define jbm_now_ms hs_now_ms
#define jbm_sleep_ms hs_sleep_ms
#define jbm_reboot_cmd hs_reboot_cmd

#else /* freestanding aarch64 */

#define SYS_ioctl 29
#define SYS_mknodat 33
#define SYS_mkdirat 34
#define SYS_umount2 39
#define SYS_mount 40
#define SYS_openat 56
#define SYS_close 57
#define SYS_getdents64 61
#define SYS_lseek 62
#define SYS_read 63
#define SYS_write 64
#define SYS_ppoll 73
#define SYS_fsync 82
#define SYS_exit_group 94
#define SYS_nanosleep 101
#define SYS_clock_gettime 113
#define SYS_sched_yield 124
#define SYS_reboot 142
#define SYS_munmap 215
#define SYS_syncfs 267
#define SYS_execve 221
#define SYS_mmap 222

#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2
#define O_CREAT 64
#define O_EXCL 128
#define O_TRUNC 512
#define O_NONBLOCK 2048

#define SEEK_SET 0
#define SEEK_END 2

#define PROT_READ 1
#define PROT_WRITE 2
#define MAP_SHARED 1
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 32

#define POLLIN 0x001

#define EV_SYN 0x00
#define EV_KEY 0x01
#define EV_ABS 0x03
#define ABS_X 0x00
#define ABS_Y 0x01
#define ABS_MT_SLOT 0x2F
#define ABS_MT_POSITION_X 0x35
#define ABS_MT_POSITION_Y 0x36
#define ABS_MT_TRACKING_ID 0x39
#define BTN_TOUCH 0x14A

#define FBIOGET_VSCREENINFO 0x4600
#define FBIOPUT_VSCREENINFO 0x4601
#define FBIOBLANK 0x4611
#define FB_ACTIVATE_FORCE 64
#define FB_ACTIVATE_ALL 32
#define FBIOGET_FSCREENINFO 0x4602

#ifndef S_IFCHR
#define S_IFCHR 0020000
#endif

#define _IOC(dir, type, nr, size) (((dir) << 30) | ((size) << 16) | ((type) << 8) | (nr))
#define _IOC_READ 2
#define EVIOCGNAME(len) _IOC(_IOC_READ, 'E', 0x06, len)
#define EVIOCGABS(code) _IOC(_IOC_READ, 'E', 0x40 + (code), sizeof(struct jbm_absinfo))

static long sc6(long n, long a, long b, long c, long d, long e, long f) {
    register long x8 __asm__("x8") = n;
    register long x0 __asm__("x0") = a;
    register long x1 __asm__("x1") = b;
    register long x2 __asm__("x2") = c;
    register long x3 __asm__("x3") = d;
    register long x4 __asm__("x4") = e;
    register long x5 __asm__("x5") = f;
    __asm__ volatile("svc 0" : "+r"(x0)
                     : "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5), "r"(x8)
                     : "memory", "cc");
    return x0;
}
static long sc5(long n, long a, long b, long c, long d, long e) { return sc6(n, a, b, c, d, e, 0); }
static long sc4(long n, long a, long b, long c, long d) { return sc6(n, a, b, c, d, 0, 0); }
static long sc3(long n, long a, long b, long c) { return sc6(n, a, b, c, 0, 0, 0); }
static long sc2(long n, long a, long b) { return sc6(n, a, b, 0, 0, 0, 0); }
static long sc1(long n, long a) { return sc6(n, a, 0, 0, 0, 0, 0); }

static int jbm_open(const char *p, int fl, int md) { return (int)sc4(SYS_openat, AT_FDCWD, (long)p, fl, md); }
static i64 jbm_read(int fd, void *b, u32 n) { return sc3(SYS_read, fd, (long)b, n); }
static i64 jbm_write(int fd, const void *b, u32 n) { return sc3(SYS_write, fd, (long)b, n); }
static int jbm_close(int fd) { return (int)sc1(SYS_close, fd); }
static int jbm_ioctl(int fd, unsigned long r, void *a) { return (int)sc3(SYS_ioctl, fd, (long)r, (long)a); }
static void *jbm_mmap_raw(u32 len, int prot, int flags, int fd, long off) {
    return (void *)sc6(SYS_mmap, 0, len, prot, flags, fd, off);
}
static void *jbm_mmap(u32 len, int prot, int flags, int fd, long off) {
    void *p = jbm_mmap_raw(len, prot, flags, fd, off);
    long e = (long)p;
    jbm_mmap_err = 0;
    if (e < 0 && e > -4096) {
        jbm_mmap_err = (int)-e; /* kernel returned -errno */
        return 0;
    }
    return p;
}
static i64 jbm_getdents(int fd, void *b, u32 n) { return sc3(SYS_getdents64, fd, (long)b, n); }
static i64 jbm_mkdir(const char *p, int m) { return sc3(SYS_mkdirat, AT_FDCWD, (long)p, m); }
static i64 jbm_mknod(const char *p, int m, int d) { return sc4(SYS_mknodat, AT_FDCWD, (long)p, m, d); }
static int jbm_mount(const char *s, const char *t, const char *ty, u64 fl, const void *d) {
    return (int)sc5(SYS_mount, (long)s, (long)t, (long)ty, fl, (long)d);
}
static int jbm_umount(const char *t) { return (int)sc2(SYS_umount2, (long)t, 0); }
static i64 jbm_pollfd_ts(void *fds, u32 n, int t) {
    /* aarch64 has no poll(2); ppoll(2) takes the timeout as a timespec. */
    struct jbm_timespec ts;
    struct jbm_timespec *tp = 0;
    if (t >= 0) {
        ts.tv_sec = t / 1000;
        ts.tv_nsec = (long)(t % 1000) * 1000000L;
        tp = &ts;
    }
    return sc5(SYS_ppoll, (long)fds, n, (long)tp, 0, 0);
}
static i64 jbm_now_ms(void) {
    struct jbm_timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 0;
    sc2(SYS_clock_gettime, 1, (long)&ts);
    return ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}
static void jbm_sleep_ms(u32 ms) {
    struct jbm_timespec ts;
    ts.tv_sec = (i64)(ms / 1000);
    ts.tv_nsec = (long)(ms % 1000) * 1000000L;
    sc2(SYS_nanosleep, (long)&ts, 0);
}
static i64 jbm_reboot_cmd(int cmd, int sub) {
    (void)sub;
    switch (cmd) {
        case JB_RB_RESTART:
            return sc4(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                       LINUX_REBOOT_CMD_RESTART, 0);
        case JB_RB_BOOTLOADER:
            return sc4(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                       LINUX_REBOOT_CMD_RESTART2, (long)"bootloader");
        case JB_RB_DOWNLOAD:
            return sc4(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                       LINUX_REBOOT_CMD_RESTART2, (long)"download");
        case JB_RB_RECOVERY:
            return sc4(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                       LINUX_REBOOT_CMD_RESTART2, (long)"recovery");
        default:
            return sc4(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2,
                       LINUX_REBOOT_CMD_POWER_OFF, 0);
    }
}

#endif /* JBM_HOST */

struct jbm_pollfd {
    int fd;
    short events;
    short revents;
};

struct jbm_absinfo {
    i32 value, minimum, maximum, fuzz, flat, resolution;
};

struct jbm_input_event {
    i64 tv_sec;
    i64 tv_usec;
    u16 type;
    u16 code;
    i32 value;
};

struct jbm_dirent64 {
    u64 d_ino;
    i64 d_off;
    u16 d_reclen;
    u8 d_type;
    char d_name[];
};

struct jbm_fb_var {
    /* Field order/size must match the kernel's struct fb_var_screeninfo
     * (160 bytes on 64-bit); offsets are checked by the static asserts below. */
    u32 xres, yres, xres_virtual, yres_virtual;
    u32 xoffset, yoffset, bits_per_pixel, grayscale;
    u32 red_offset, red_length, red_msb_right;
    u32 green_offset, green_length, green_msb_right;
    u32 blue_offset, blue_length, blue_msb_right;
    u32 transp_offset, transp_length, transp_msb_right;
    u32 nonstd, activate, height, width;
    u32 accel_flags, pixclock;
    u32 left_margin, right_margin, upper_margin, lower_margin;
    u32 hsync_len, vsync_len, sync, vmode;
    u32 rotate, colorspace;
    u32 reserved[4];
};

struct jbm_fb_fix {
    char id[16];
    u64 smem_start;
    u32 smem_len;
    u32 type, type_aux, visual;
    u16 xpanstep, ypanstep, ywrapstep;
    u32 line_length;
    u64 mmio_start;
    u32 mmio_len;
    u32 accel;
    u16 capabilities;
    u16 reserved[2];
};

_Static_assert(sizeof(struct jbm_input_event) == 24, "input_event size");
_Static_assert(sizeof(struct jbm_fb_var) == 160, "fb_var size");
_Static_assert(sizeof(struct jbm_fb_fix) == 80, "fb_fix size");
_Static_assert(sizeof(struct jbm_absinfo) == 24, "absinfo size");
_Static_assert(__builtin_offsetof(struct jbm_fb_var, red_offset) == 32, "fb_var red");
_Static_assert(__builtin_offsetof(struct jbm_fb_var, transp_offset) == 68, "fb_var transp");
_Static_assert(__builtin_offsetof(struct jbm_fb_fix, line_length) == 48, "fb_fix line_length");

/* ------------------------------------------------------------------ */
/* libc                                                                */
/* ------------------------------------------------------------------ */

#ifndef JBM_HOST
void *memset(void *d, int c, unsigned long n);
void *memcpy(void *d, const void *s, unsigned long n);
void *memmove(void *d, const void *s, unsigned long n);
int memcmp(const void *a, const void *b, unsigned long n);
#endif

void *memset(void *d, int c, unsigned long n) {
    u8 *p = (u8 *)d;
    while (n--) *p++ = (u8)c;
    return d;
}

void *memcpy(void *d, const void *s, unsigned long n) {
    u8 *dp = (u8 *)d;
    const u8 *sp = (const u8 *)s;
    if (!n || dp == sp) return d;
    if (((u64)dp & 7) == 0 && ((u64)sp & 7) == 0) {
        while (n && ((u64)dp & 7)) {
            *dp++ = *sp++;
            n--;
        }
        while (n >= 8) {
            *(u64 *)dp = *(const u64 *)sp;
            dp += 8;
            sp += 8;
            n -= 8;
        }
    }
    while (n--) *dp++ = *sp++;
    return d;
}

void *memmove(void *d, const void *s, unsigned long n) {
    u8 *dp = (u8 *)d;
    const u8 *sp = (const u8 *)s;
    if (dp == sp || !n) return d;
    if (dp < sp) return memcpy(d, s, n);
    dp += n;
    sp += n;
    while (n--) *--dp = *--sp;
    return d;
}

int memcmp(const void *a, const void *b, unsigned long n) {
    const u8 *x = (const u8 *)a, *y = (const u8 *)b;
    while (n--) {
        if (*x != *y) return (int)*x - (int)*y;
        x++;
        y++;
    }
    return 0;
}

static int jbm_str_has(const char *hay, const char *needle) {
    if (!*needle) return 1;
    for (const char *h = hay; *h; h++) {
        const char *a = h;
        const char *b = needle;
        while (*a && *b && *a == *b) {
            a++;
            b++;
        }
        if (!*b) return 1;
    }
    return 0;
}

static void jbm_strcpy_(char *d, const char *s, u32 cap) {
    u32 i = 0;
    if (!cap) return;
    while (s[i] && i < cap - 1) {
        d[i] = s[i];
        i++;
    }
    d[i] = 0;
}

#ifndef JBM_HOST
static int jbm_atoi_(const char *s) {
    int v = 0, neg = 0;
    if (*s == '-') {
        neg = 1;
        s++;
    }
    while (*s >= '0' && *s <= '9') v = v * 10 + (*s++ - '0');
    return neg ? -v : v;
}
#endif

static i32 jbm_isqrt(i32 v) {
    if (v <= 0) return 0;
    i32 r = v, x = (v / 2) + 1;
    while (x < r) {
        r = x;
        x = (x + v / x) / 2;
    }
    return r;
}

static char *jbm_fmtd(char *o, i64 v, u32 pad, char fill) {
    char tmp[24];
    int n = 0;
    u64 u = v < 0 ? (u64)(-v) : (u64)v;
    if (!u) tmp[n++] = '0';
    while (u) {
        tmp[n++] = (char)('0' + (u % 10));
        u /= 10;
    }
    while ((u32)n < pad) tmp[n++] = fill;
    if (v < 0) tmp[n++] = '-';
    for (int i = n - 1; i >= 0; i--) *o++ = tmp[i];
    *o = 0;
    return o;
}

static int jbm_vfmt(char *out, const char *fmt, va_list ap) {
    char *o = out;
    while (*fmt) {
        if (*fmt != '%') {
            *o++ = *fmt++;
            continue;
        }
        fmt++;
        char padc = ' ';
        u32 pad = 0;
        if (*fmt == '0') {
            padc = '0';
            fmt++;
        }
        while (*fmt >= '0' && *fmt <= '9') {
            pad = pad * 10 + (u32)(*fmt++ - '0');
        }
        while (*fmt == 'l' || *fmt == 'z' || *fmt == 'j' || *fmt == 'h') fmt++;
        switch (*fmt++) {
            case 'd':
            case 'i':
                o = jbm_fmtd(o, va_arg(ap, i64), pad, padc);
                break;
            case 'u':
                o = jbm_fmtd(o, (i64)va_arg(ap, unsigned int), pad, padc);
                break;
            case 's': {
                const char *s = va_arg(ap, const char *);
                if (!s) s = "(null)";
                while (*s) *o++ = *s++;
                break;
            }
            case 'c':
                *o++ = (char)va_arg(ap, int);
                break;
            case '%':
                *o++ = '%';
                break;
            default:
                *o++ = '?';
                break;
        }
    }
    *o = 0;
    return (int)(o - out);
}

static int jbm_sn(char *out, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = jbm_vfmt(out, fmt, ap);
    va_end(ap);
    return n;
}

static int jbm_klog_fd = -1;
static int jbm_kmsg_err = 0;
static int jbm_con_err = 0;

static void jbm_log(const char *fmt, ...) {
    char b[512];
    char l[600];
    va_list ap;
    va_start(ap, fmt);
    jbm_vfmt(b, fmt, ap);
    va_end(ap);
    if (jbm_klog_fd < 0) {
        /* Prefer /dev/kmsg: those lines land in the kernel log, so they show
         * up in dmesg and survive in pstore console-ramoops after reboot.
         * /dev/console alone is a VT write and is lost. */
        jbm_klog_fd = jbm_open("/dev/kmsg", O_WRONLY, 0);
        if (jbm_klog_fd >= 0) {
            jbm_kmsg_err = 0;
        } else {
            jbm_kmsg_err = -jbm_klog_fd;
            jbm_klog_fd = jbm_open("/dev/console", O_WRONLY, 0);
            jbm_con_err = jbm_klog_fd < 0 ? -jbm_klog_fd : 0;
        }
    }
    int n = 0;
    l[n++] = '<';
    l[n++] = '6';
    l[n++] = '>';
    for (int i = 0; b[i] && n < (int)sizeof(l) - 40; i++) l[n++] = b[i];
    if (jbm_klog_fd < 0 && jbm_kmsg_err) {
        n += jbm_sn(l + n, " [kmsg_err=%d con_err=%d]", jbm_kmsg_err, jbm_con_err);
    }
    l[n++] = '\n';
    if (jbm_klog_fd >= 0)
        jbm_write(jbm_klog_fd, l, (u32)n);
    else
        jbm_write(1, l, (u32)n); /* stdout only when kmsg is unavailable */
}

#ifndef JBM_HOST
static long jbm_read_file(const char *path, char *buf, u32 cap) {
    int fd = jbm_open(path, O_RDONLY, 0);
    if (fd < 0) return -1;
    long n = jbm_read(fd, buf, cap - 1);
    jbm_close(fd);
    if (n < 0) return -1;
    if (n > (long)cap - 1) n = (long)cap - 1;
    buf[n] = 0;
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r' || buf[n - 1] == ' ')) buf[--n] = 0;
    return n;
}
#endif

static void jbm_sync_all(void) {
    int fd = jbm_open("/", O_RDONLY, 0);
    if (fd >= 0) {
#ifdef JBM_HOST
        jbm_fsync(fd);
#else
        sc1(SYS_syncfs, fd);
#endif
        jbm_close(fd);
    }
}

/* ------------------------------------------------------------------ */
/* canvas / framebuffer                                                */
/* ------------------------------------------------------------------ */

struct gfx {
    int fd;
    u8 *mem;
    u8 *cv;
    u8 *bg;
    u32 stride;
    u32 len;
    u32 w, h, bpp;
    u32 ro, rl, go, gl, bo, bl, to, tl;
    int ok;
};

static struct gfx G;

/* Scales an 8-bit colour component down to a framebuffer channel of `len` bits. */
static u32 chan_down(u32 v, u32 len) {
    if (len >= 8) return v;
    if (len == 0) return 0;
    return (v * (((u32)1 << len) - 1) + 127) / 255;
}

/* Expands a framebuffer channel of `len` bits up to 8 bits. */
static u32 chan_up(u32 v, u32 len) {
    if (len == 0) return 0;
    if (len >= 8) return v & 0xFF;
    u32 m = ((u32)1 << len) - 1;
    return (v & m) * 255 / m;
}

/* Packs an ARGB8888 value into the native framebuffer pixel layout. */
static u32 pack_px(u32 argb) {
    u32 v = (chan_down((argb >> 16) & 0xFF, G.rl) << G.ro) |
            (chan_down((argb >> 8) & 0xFF, G.gl) << G.go) |
            (chan_down(argb & 0xFF, G.bl) << G.bo);
    if (G.tl) v |= chan_down((argb >> 24) & 0xFF, G.tl) << G.to;
    return v;
}

/* Unpacks a native framebuffer pixel back to ARGB8888. */
static u32 unpack_px(u32 raw) {
    u32 a = G.tl ? chan_up((raw >> G.to), G.tl) : 0xFF;
    return (a << 24) | (chan_up((raw >> G.ro), G.rl) << 16) | (chan_up((raw >> G.go), G.gl) << 8) |
           chan_up((raw >> G.bo), G.bl);
}

/* Rejects channel descriptions that cannot describe a real pixel format. */
static int chan_ok(u32 off, u32 len, u32 bpp) {
    if (len == 0 || len > 8) return 0;
    if (off + len > bpp) return 0;
    return 1;
}

/* errno (positive) of the last gfx_open() failure, 0 if the failure was a
 * validation reject rather than a syscall error. */
static int gfx_err = 0;
/* Suppress per-attempt failure lines while the caller is polling gfx_open(). */
static int gfx_quiet = 0;

static int gfx_open(void) {
    static char nodes[4][40];
    int fd = -1;
    gfx_err = 0;
    for (int i = 0; i < 4; i++) {
        jbm_sn(nodes[i], "/dev/fb%d", i);
        fd = jbm_open(nodes[i], O_RDWR, 0);
        if (fd >= 0) break;
        if (fd < 0 && -fd > gfx_err) gfx_err = -fd;
    }
    if (fd < 0) {
        jbm_mkdir("/dev", 0755);
        jbm_mkdir("/dev/graphics", 0755);
        i64 mk = jbm_mknod("/dev/graphics/fb0", 0600 | S_IFCHR, (29 << 8) | 0);
        fd = jbm_open("/dev/graphics/fb0", O_RDWR, 0);
        if (fd < 0) {
            gfx_err = -fd;
            if (mk < 0) jbm_log("jbm: mknod /dev/graphics/fb0 failed (%d)", (int)-mk);
        }
    }
    if (fd < 0) {
        if (!gfx_quiet) jbm_log("jbm: no framebuffer node (err %d)", gfx_err);
        return -1;
    }

    struct jbm_fb_var var;
    struct jbm_fb_fix fix;
    memset(&var, 0, sizeof(var));
    memset(&fix, 0, sizeof(fix));

    i64 vr = jbm_ioctl(fd, FBIOGET_VSCREENINFO, &var);
    if (vr != 0 || var.xres < 16 || var.xres > 8192 || var.yres < 16 || var.yres > 8192) {
        gfx_err = vr < 0 ? (int)-vr : 0;
        if (!gfx_quiet)
            jbm_log("jbm: FBIOGET_VSCREENINFO unusable (err %d xres=%u yres=%u bpp=%u)", gfx_err, var.xres,
                    var.yres, var.bits_per_pixel);
        jbm_close(fd);
        return -1;
    }
    int have_fix = jbm_ioctl(fd, FBIOGET_FSCREENINFO, &fix) == 0;
    if (var.bits_per_pixel != 32 && var.bits_per_pixel != 16) {
        jbm_log("jbm: unsupported bpp %u", var.bits_per_pixel);
        jbm_close(fd);
        return -1;
    }

    u32 stride = (have_fix && fix.line_length) ? fix.line_length : var.xres * (var.bits_per_pixel / 8);
    u32 len = (have_fix && fix.smem_len) ? fix.smem_len : stride * var.yres_virtual;
    if (stride < var.xres * (var.bits_per_pixel / 8) || len < stride * var.yres) {
        jbm_log("jbm: framebuffer geometry inconsistent");
        jbm_close(fd);
        return -1;
    }

    if (!chan_ok(var.red_offset, var.red_length, var.bits_per_pixel) ||
        !chan_ok(var.green_offset, var.green_length, var.bits_per_pixel) ||
        !chan_ok(var.blue_offset, var.blue_length, var.bits_per_pixel) ||
        (var.transp_length && !chan_ok(var.transp_offset, var.transp_length, var.bits_per_pixel))) {
        jbm_log("jbm: unusable fb channel layout r=%u/%u g=%u/%u b=%u/%u", var.red_offset, var.red_length,
                var.green_offset, var.green_length, var.blue_offset, var.blue_length);
        jbm_close(fd);
        return -1;
    }

    u8 *mem = (u8 *)jbm_mmap(len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (!mem) {
        gfx_err = jbm_mmap_err;
        if (!gfx_quiet) jbm_log("jbm: fb mmap failed (err %d len=%u)", gfx_err, len);
        jbm_close(fd);
        return -1;
    }
    u8 *cv = (u8 *)jbm_mmap(stride * var.yres, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!cv) {
        jbm_log("jbm: canvas alloc failed");
        jbm_close(fd);
        return -1;
    }
    /* The sky and the planet never change, so they are painted once into their
     * own buffer and copied back into the canvas each frame; only the strip of
     * screen the menus live in is redrawn. */
    u8 *bg = (u8 *)jbm_mmap(stride * var.yres, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (!bg) {
        jbm_log("jbm: background alloc failed");
        jbm_close(fd);
        return -1;
    }

    G.fd = fd;
    G.mem = mem;
    G.cv = cv;
    G.bg = bg;
    G.stride = stride;
    G.len = len;
    G.w = var.xres;
    G.h = var.yres;
    G.bpp = var.bits_per_pixel;
    G.ro = var.red_offset;
    G.rl = var.red_length;
    G.go = var.green_offset;
    G.gl = var.green_length;
    G.bo = var.blue_offset;
    G.bl = var.blue_length;
    G.to = var.transp_offset;
    G.tl = var.transp_length;
    G.ok = 1;
    jbm_log("jbm: fb %ux%u bpp=%u stride=%u len=%u rgb=%u:%u/%u:%u/%u:%u a=%u:%u", G.w, G.h, G.bpp, stride, len,
            G.rl, G.ro, G.gl, G.go, G.bl, G.bo, G.tl, G.to);
    return 0;
}

/* mtkfb keeps scanning whatever LK left programmed in its layer: writing the
 * framebuffer is not enough, the driver has to re-apply the mode for the layer
 * to pick up the new contents. Verified on this device: FBIOPUT_VSCREENINFO with
 * FB_ACTIVATE_FORCE is what makes fb0 visible (without it the boot logo stays
 * frozen on screen while the menu draws into memory nobody reads). */
static void gfx_kick(void) {
    if (!G.ok) return;
    struct jbm_fb_var var;
    memset(&var, 0, sizeof(var));
    i64 g = jbm_ioctl(G.fd, FBIOGET_VSCREENINFO, &var);
    var.activate = FB_ACTIVATE_FORCE | FB_ACTIVATE_ALL;
    i64 p = jbm_ioctl(G.fd, FBIOPUT_VSCREENINFO, &var);
    i64 b1 = jbm_ioctl(G.fd, FBIOBLANK, (void *)1);
    jbm_sleep_ms(250);
    i64 b0 = jbm_ioctl(G.fd, FBIOBLANK, (void *)0);
    jbm_log("jbm: gfx kick get=%d put=%d blank1=%d blank0=%d", (int)g, (int)p, (int)b1, (int)b0);
}

static u32 mix(u32 c1, u32 c2, u32 t) {
    u32 r = (((c1 >> 16) & 0xFF) * (255 - t) + ((c2 >> 16) & 0xFF) * t) / 255;
    u32 g = (((c1 >> 8) & 0xFF) * (255 - t) + ((c2 >> 8) & 0xFF) * t) / 255;
    u32 b = ((c1 & 0xFF) * (255 - t) + (c2 & 0xFF) * t) / 255;
    return (r << 16) | (g << 8) | b;
}

/* Drawing colours are ARGB (0xAARRGGBB); these literals are written RGBA
 * (0xRRGGBBAA) for readability, so convert them once. */
#define RGBA(v) ((u32)((((u32)(v) & 0xFFu) << 24) | ((u32)(v) >> 8)))

static inline void store_px(int x, int y, u32 argb) {
    u8 *row = G.cv + (u32)y * G.stride;
    if (G.bpp == 16) {
        *(u16 *)(row + (u32)x * 2) = (u16)pack_px(argb);
    } else {
        *(u32 *)(row + (u32)x * 4) = pack_px(argb);
    }
}

static inline u32 load_px(int x, int y) {
    u8 *row = G.cv + (u32)y * G.stride;
    u32 raw = G.bpp == 16 ? (u32)*(u16 *)(row + (u32)x * 2) : *(u32 *)(row + (u32)x * 4);
    return unpack_px(raw);
}

/* Alpha-composites argb onto the canvas. Returns the resulting ARGB value. */
static inline u32 px(int x, int y, u32 argb) {
    if ((u32)x >= G.w || (u32)y >= G.h) return 0;
    u32 a = (argb >> 24) & 0xFF;
    if (!a) return load_px(x, y);
    if (a == 255) {
        store_px(x, y, argb);
        return argb;
    }
    u32 d = load_px(x, y);
    u32 da = d >> 24;
    u32 out_a = a + da * (255 - a) / 255;
    if (!out_a) {
        store_px(x, y, 0);
        return 0;
    }
    u32 k = (255 - a) * da / 255;
    u32 r = (((argb >> 16) & 0xFF) * a + ((d >> 16) & 0xFF) * k) / out_a;
    u32 g = (((argb >> 8) & 0xFF) * a + ((d >> 8) & 0xFF) * k) / out_a;
    u32 b = ((argb & 0xFF) * a + (d & 0xFF) * k) / out_a;
    u32 out = (out_a << 24) | (r << 16) | (g << 8) | b;
    store_px(x, y, out);
    return out;
}

static void blend(int x, int y, u32 argb) { px(x, y, argb); }

static void fill_rect(int x, int y, int w, int h, u32 argb) {
    if (w <= 0 || h <= 0) return;
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w, y1 = y + h;
    if (x1 > (int)G.w) x1 = (int)G.w;
    if (y1 > (int)G.h) y1 = (int)G.h;
    if (x1 <= x0 || y1 <= y0) return;

    if ((argb >> 24) == 255) {
        u32 v = pack_px(argb);
        if (G.bpp == 16) {
            for (int yy = y0; yy < y1; yy++) {
                u16 *p = (u16 *)(G.cv + (u32)yy * G.stride) + x0;
                for (int xx = x0; xx < x1; xx++) *p++ = (u16)v;
            }
        } else {
            for (int yy = y0; yy < y1; yy++) {
                u32 *p = (u32 *)(G.cv + (u32)yy * G.stride) + x0;
                for (int xx = x0; xx < x1; xx++) *p++ = v;
            }
        }
        return;
    }
    for (int yy = y0; yy < y1; yy++)
        for (int xx = x0; xx < x1; xx++) blend(xx, yy, argb);
}

static int rr_inset(int r, int row) {
    int d = r - 1 - row;
    int half = jbm_isqrt(r * r - d * d);
    int inset = r - half;
    return inset < 0 ? 0 : inset;
}

static void round_rect(int x, int y, int w, int h, int r, u32 argb) {
    if (w <= 0 || h <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r < 0) r = 0;
    for (int row = 0; row < h; row++) {
        int e = row < h - 1 - row ? row : h - 1 - row;
        int in = (r > 0 && e < r) ? rr_inset(r, e) : 0;
        fill_rect(x + in, y + row, w - 2 * in, 1, argb);
    }
}

static void round_rect_border(int x, int y, int w, int h, int r, int th, u32 argb) {
    if (w <= 0 || h <= 0 || th <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r < 0) r = 0;
    int ri = r - th;
    if (ri < 0) ri = 0;
    for (int row = 0; row < h; row++) {
        int e = row < h - 1 - row ? row : h - 1 - row;
        int o = (r > 0 && e < r) ? rr_inset(r, e) : 0;
        if (row < th || row >= h - th) {
            fill_rect(x + o, y + row, w - 2 * o, 1, argb);
            continue;
        }
        int ei = row - th < h - 1 - th - row ? row - th : h - 1 - th - row;
        int ii = th + ((ri > 0 && ei < ri) ? rr_inset(ri, ei) : 0);
        if (ii <= o) ii = o + 1;
        fill_rect(x + o, y + row, ii - o, 1, argb);
        fill_rect(x + w - ii, y + row, ii - o, 1, argb);
    }
}

static void disc(int cx, int cy, int r, u32 c) {
    if (r <= 0) return;
    for (int dy = -r; dy <= r; dy++) {
        int s = jbm_isqrt(r * r - dy * dy);
        fill_rect(cx - s, cy + dy, s * 2 + 1, 1, c);
    }
}

static void ring(int cx, int cy, int r, int th, u32 c) {
    if (th <= 0 || r <= 0) return;
    int ri = r - th;
    if (ri < 0) ri = 0;
    for (int dy = -r; dy <= r; dy++) {
        int s = jbm_isqrt(r * r - dy * dy);
        if (dy * dy < ri * ri) {
            int s2 = jbm_isqrt(ri * ri - dy * dy);
            fill_rect(cx - s, cy + dy, s - s2, 1, c);
            fill_rect(cx + s2 + 1, cy + dy, s - s2, 1, c);
        } else {
            fill_rect(cx - s, cy + dy, s * 2 + 1, 1, c);
        }
    }
}

static float jbm_sinf(float a) {
    const float pi = 3.14159265358979f;
    while (a > pi) a -= 2.0f * pi;
    while (a < -pi) a += 2.0f * pi;
    float x = a;
    float x2 = x * x;
    return x * (1.0f - x2 / 6.0f * (1.0f - x2 / 20.0f * (1.0f - x2 / 42.0f)));
}

static void arc(int cx, int cy, int r, int th, int a0, int a1, u32 c) {
    if (a1 <= a0 || r <= 0) return;
    int steps = ((a1 - a0) * r) / 40 + 8;
    for (int i = 0; i <= steps; i++) {
        float rad = 3.14159265358979f * (float)(a0 + (a1 - a0) * i / steps) / 180.0f;
        int dx = cx + (int)(jbm_sinf(rad + 1.5707963f) * (float)r);
        int dy = cy - (int)(jbm_sinf(rad) * (float)r);
        disc(dx, dy, th / 2 < 1 ? 1 : th / 2, c);
    }
}

/* ------------------------------------------------------------------ */
/* text                                                                */
/* ------------------------------------------------------------------ */

struct font {
    const char *data;
    const short (*meta)[7];
    int aw, ah, px, top, bot, first, count;
};

static struct font FN20 = {F20_data, F20_meta, F20_ATLAS_W, F20_ATLAS_H, F20_PX,
                           F20_TOP,   F20_BOT,   F20_FIRST, F20_COUNT};
static struct font FN32 = {F34_data, F34_meta, F34_ATLAS_W, F34_ATLAS_H, F34_PX,
                           F34_TOP,   F34_BOT,   F34_FIRST, F34_COUNT};
static struct font FN48 = {F48_data, F48_meta, F48_ATLAS_W, F48_ATLAS_H, F48_PX,
                           F48_TOP,   F48_BOT,   F48_FIRST, F48_COUNT};
static struct font FDIG = {FDIG_data, FDIG_meta, FDIG_ATLAS_W, FDIG_ATLAS_H, FDIG_PX,
                           FDIG_TOP,  FDIG_BOT,  FDIG_FIRST, FDIG_COUNT};

#define TA_L 0
#define TA_C 1
#define TA_R 2
#define VA_T 0
#define VA_M 4
#define VA_B 8

static float glyph_adv(struct font *f, int idx, float k) { return (float)f->meta[idx][6] * k; }

static float scale_of(struct font *f, float px) { return px / (float)f->px; }

/* The atlases only hold ASCII, so the middle dot used as a separator in the
 * design is drawn instead of rasterised. */
#define GLYPH_DOT 0xB7

/* Advance of one character in pixels, -1 when the font cannot draw it. */
static int char_adv(struct font *f, int ch, float px) {
    if (ch == GLYPH_DOT) return (int)(px * 0.34f + 0.5f);
    int i = ch - f->first;
    if (i < 0 || i >= f->count) return -1;
    return (int)(glyph_adv(f, i, scale_of(f, px)) + 0.5f);
}

static int measure_ls(struct font *f, const char *s, float px, int ls) {
    int w = 0;
    for (const char *p = s; *p; p++) {
        int a = char_adv(f, (int)(u8)*p, px);
        if (a < 0) continue;
        w += a + ls;
    }
    return w > 0 ? w - ls : 0;
}

static int line_h(float px) { return (int)(px * 1.32f + 0.5f); }

static void draw_text_ls(struct font *f, int x, int y, float px, u32 argb, const char *s, int align,
                         int ls) {
    if (!f->count) return;
    float k = scale_of(f, px);
    int lh = line_h(px);
    if (align & TA_C) x -= measure_ls(f, s, px, ls) / 2;
    else if (align & TA_R) x -= measure_ls(f, s, px, ls);
    if (align & VA_M) y -= lh / 2;
    else if (align & VA_B) y -= lh;

    u32 alpha = argb >> 24;
    u32 rgb = argb & 0xFFFFFF;

    for (const char *p = s; *p; p++) {
        int ch = (int)(u8)*p;
        if (ch == GLYPH_DOT) {
            int rr = (int)(px / 26.0f);
            if (rr < 1) rr = 1;
            disc(x + (int)(px * 0.17f) + ls / 2, y + (int)(px * 0.55f), rr, (alpha << 24) | rgb);
            x += char_adv(f, ch, px) + ls;
            continue;
        }
        int idx = ch - f->first;
        if (idx < 0 || idx >= f->count) continue;
        const short *m = f->meta[idx];
        int gw = m[2], gh = m[3];
        if (gw > 0 && gh > 0) {
            int sx = m[0], sy = m[1];
            float dw = (float)gw * k, dh = (float)gh * k;
            int dw2 = (int)(dw + 0.5f), dh2 = (int)(dh + 0.5f);
            if (dw2 < 1) dw2 = 1;
            if (dh2 < 1) dh2 = 1;
            int ox = x + (int)((float)m[4] * k);
            int oy = y + (int)((float)m[5] * k);
            float stepx = (float)gw / (float)dw2, stepy = (float)gh / (float)dh2;
            for (int yy = 0; yy < dh2; yy++) {
                float fy = ((float)yy + 0.5f) * stepy - 0.5f;
                int iy = (int)fy;
                float fy2 = fy - (float)iy;
                for (int xx = 0; xx < dw2; xx++) {
                    float fx = ((float)xx + 0.5f) * stepx - 0.5f;
                    int ix = (int)fx;
                    float fx2 = fx - (float)ix;
                    int ax0 = ix < 0 ? 0 : ix, ax1 = (ix + 1 >= gw) ? gw - 1 : ix + 1;
                    int ay0 = iy < 0 ? 0 : iy, ay1 = (iy + 1 >= gh) ? gh - 1 : iy + 1;
                    float v00 = (float)(u8)f->data[(sy + ay0) * f->aw + (sx + ax0)];
                    float v10 = (float)(u8)f->data[(sy + ay0) * f->aw + (sx + ax1)];
                    float v01 = (float)(u8)f->data[(sy + ay1) * f->aw + (sx + ax0)];
                    float v11 = (float)(u8)f->data[(sy + ay1) * f->aw + (sx + ax1)];
                    float a = v00 + (v10 - v00) * fx2;
                    float b = v01 + (v11 - v01) * fx2;
                    a = a + (b - a) * fy2;
                    if (a < 1.0f) continue;
                    u32 na = (u32)(a * (float)alpha / 255.0f + 0.5f);
                    if (!na) continue;
                    blend(ox + xx, oy + yy, (na << 24) | rgb);
                }
            }
        }
        x += (int)(glyph_adv(f, idx, k) + 0.5f) + ls;
    }
}

static void draw_text(struct font *f, int x, int y, float px, u32 argb, const char *s, int align) {
    draw_text_ls(f, x, y, px, argb, s, align, 0);
}

/* Two runs of text in two colours, laid out and aligned as a single string. */
static void draw_pair(struct font *f, int x, int y, float px, int align, int ls, u32 c1, const char *s1,
                      u32 c2, const char *s2) {
    int w1 = measure_ls(f, s1, px, ls);
    int w2 = measure_ls(f, s2, px, ls);
    int lh = line_h(px);
    if (align & TA_C) x -= (w1 + w2) / 2;
    else if (align & TA_R) x -= w1 + w2;
    if (align & VA_M) y -= lh / 2;
    else if (align & VA_B) y -= lh;
    draw_text_ls(f, x, y, px, c1, s1, TA_L, ls);
    draw_text_ls(f, x + w1, y, px, c2, s2, TA_L, ls);
}

/* ------------------------------------------------------------------ */
/* touch input                                                         */
/* ------------------------------------------------------------------ */

struct touch {
    int fd;
    int slot, active, ranged, mt;
    i32 rx, ry, x0, x1, y0, y1;
    int sx, sy;
    int down_x, down_y;
    i64 down_t;
    int holding, pressed, released;
    int tap_x, tap_y;
};

static struct touch T = {.fd = -1};

static int touch_open(void) {
    static char ents[32][64];
    int count = 0;
    #ifdef JBM_HOST
    const char *dir = "/dev/input";
#else
    const char *dir = "/sys/class/input";
#endif
    {
        int fd = jbm_open(dir, O_RDONLY, 0);
        if (fd >= 0) {
            static u8 buf[8192];
            for (;;) {
                long r = jbm_getdents(fd, buf, (u32)sizeof(buf));
                if (r <= 0) break;
                long off = 0;
                while (off < r && count < 32) {
                    struct jbm_dirent64 *d = (struct jbm_dirent64 *)(buf + off);
                    off += d->d_reclen;
                    if (d->d_name[0] == '.') continue;
                    jbm_strcpy_(ents[count++], d->d_name, 64);
                }
            }
            jbm_close(fd);
        }
    }

    if (!count) {
        jbm_log("jbm: no input devices under %s", dir);
        return 0;
    }

    int best_fd = -1;
    int best_score = -1;
    int best_mt = 0;
    char best_nm[64];
    struct jbm_absinfo bax, bay;
    memset(best_nm, 0, sizeof(best_nm));

    for (int i = 0; i < count; i++) {
        if (!jbm_str_has(ents[i], "event")) continue;
        char node[80];
#ifdef JBM_HOST
        jbm_sn(node, "%s/%s", dir, ents[i]);
#else
        char sysp[128], raw[64];
        jbm_sn(sysp, "/sys/class/input/%s/dev", ents[i]);
        if (jbm_read_file(sysp, raw, sizeof(raw)) <= 0) continue;
        const char *colon = raw;
        while (*colon && *colon != ':') colon++;
        if (!*colon) continue;
        int maj = jbm_atoi_(raw), min = jbm_atoi_(colon + 1);

        jbm_mkdir("/dev", 0755);
        jbm_mkdir("/dev/input", 0755);
        jbm_sn(node, "/dev/input/%s", ents[i]);
        jbm_mknod(node, 0600 | S_IFCHR, (maj << 8) | min);
#endif

        int fd = jbm_open(node, O_RDONLY | O_NONBLOCK, 0);
        if (fd < 0) continue;

        char nm[64];
        memset(nm, 0, sizeof(nm));
        struct jbm_absinfo ax, ay;
        jbm_ioctl(fd, EVIOCGNAME(sizeof(nm)), nm);
        int mt = 1;
        int okx = jbm_ioctl(fd, EVIOCGABS(ABS_MT_POSITION_X), &ax) == 0;
        int oky = jbm_ioctl(fd, EVIOCGABS(ABS_MT_POSITION_Y), &ay) == 0;
        if (!okx) {
            okx = jbm_ioctl(fd, EVIOCGABS(ABS_X), &ax) == 0;
            mt = 0;
        }
        if (!oky) {
            oky = jbm_ioctl(fd, EVIOCGABS(ABS_Y), &ay) == 0;
        }
        if (!okx || !oky) {
            jbm_close(fd);
            continue;
        }

        /* A collapsed axis range cannot be mapped to screen coordinates, so
         * devices like sec_touchproximity (0..0) are never usable. */
        if ((i64)ax.maximum - (i64)ax.minimum <= 0 || (i64)ay.maximum - (i64)ay.minimum <= 0) {
            jbm_close(fd);
            continue;
        }

        /* Score every candidate instead of taking the first "touch" match:
         * getdents order is arbitrary and would otherwise pick e.g. the
         * proximity sensor over the real touchscreen. */
        int score = 1;
        if (jbm_str_has(nm, "touchscreen")) score += 100;
        else if (jbm_str_has(nm, "touch")) score += 40;
        if (jbm_str_has(nm, "proximity")) score -= 60;
        if (mt) score += 10;

        if (score > best_score) {
            if (best_fd >= 0) jbm_close(best_fd);
            best_fd = fd;
            best_score = score;
            best_mt = mt;
            memcpy(best_nm, nm, sizeof(best_nm));
            bax = ax;
            bay = ay;
        } else {
            jbm_close(fd);
        }
    }

    if (best_fd >= 0) {
        T.fd = best_fd;
        T.mt = best_mt;
        T.ranged = 1;
        T.x0 = bax.minimum;
        T.x1 = bax.maximum;
        T.y0 = bay.minimum;
        T.y1 = bay.maximum;
        T.sx = (int)G.w / 2;
        T.sy = (int)G.h / 2;
        jbm_log("jbm: touch=%s x[%d..%d] y[%d..%d] %s score=%d", best_nm[0] ? best_nm : "(noname)",
                T.x0, T.x1, T.y0, T.y1, T.mt ? "mt" : "abs", best_score);
        return 1;
    }
    jbm_log("jbm: no usable touchscreen (event nodes with ABS axes)");
    return 0;
}

static void touch_map(void) {
    i64 rx = (i64)T.x1 - T.x0, ry = (i64)T.y1 - T.y0;
    if (rx <= 0 || ry <= 0) return;
    i64 ax = T.rx, ay = T.ry;
    if (ax < T.x0) ax = T.x0;
    if (ax > T.x1) ax = T.x1;
    if (ay < T.y0) ay = T.y0;
    if (ay > T.y1) ay = T.y1;
    i64 sx = ((ax - T.x0) * ((i64)G.w - 1)) / rx;
    i64 sy = ((ay - T.y0) * ((i64)G.h - 1)) / ry;
    T.sx = (int)sx;
    T.sy = (int)sy;
}

static void touch_poll(void) {
    T.pressed = 0;
    T.released = 0;
    if (T.fd < 0) return;
    struct jbm_pollfd p;
    p.fd = T.fd;
    p.events = POLLIN;
    p.revents = 0;
    if (jbm_pollfd_ts(&p, 1, 0) <= 0) return;

    struct jbm_input_event e;
    for (;;) {
        if (jbm_read(T.fd, &e, (u32)sizeof(e)) != (i64)sizeof(e)) break;
        switch (e.type) {
            case EV_SYN:
                break;
            case EV_ABS:
                if (e.code == ABS_MT_SLOT) {
                    T.slot = e.value;
                } else if (e.code == ABS_MT_TRACKING_ID) {
                    if (e.value < 0) {
                        if (T.holding) {
                            T.holding = 0;
                            T.released = 1;
                            T.tap_x = T.sx;
                            T.tap_y = T.sy;
                        }
                    } else {
                        T.active = T.slot;
                        if (!T.holding) {
                            T.holding = 1;
                            T.pressed = 1;
                            T.down_x = T.sx;
                            T.down_y = T.sy;
                            T.down_t = jbm_now_ms();
                        }
                    }
                } else if (e.code == ABS_MT_POSITION_X || e.code == ABS_X) {
                    T.rx = e.value;
                    touch_map();
                } else if (e.code == ABS_MT_POSITION_Y || e.code == ABS_Y) {
                    T.ry = e.value;
                    touch_map();
                }
                break;
            case EV_KEY:
                if (e.code == BTN_TOUCH) {
                    if (e.value == 0) {
                        if (T.holding) {
                            T.holding = 0;
                            T.released = 1;
                            T.tap_x = T.sx;
                            T.tap_y = T.sy;
                        }
                    } else if (!T.holding) {
                        T.holding = 1;
                        T.pressed = 1;
                        T.down_x = T.sx;
                        T.down_y = T.sy;
                        T.down_t = jbm_now_ms();
                    }
                }
                break;
            default:
                break;
        }
    }
}

/* ------------------------------------------------------------------ */
/* actions                                                             */
/* ------------------------------------------------------------------ */

#define ACT_COUNT 6
enum { ACT_SYSTEM, ACT_RECOVERY, ACT_FASTBOOT, ACT_DOWNLOAD, ACT_REBOOT, ACT_POWEROFF };

/* Lower case for the log lines, upper case for the screen. */
static const char *act_label[ACT_COUNT] = {"System",    "Recovery",  "Fastboot",
                                           "Download",  "Reboot",   "Power off"};
static const char *act_label_up[ACT_COUNT] = {"SYSTEM",   "RECOVERY", "FASTBOOT",
                                               "DOWNLOAD", "REBOOT",   "POWER OFF"};
static const char *act_note[ACT_COUNT] = {"Starting Android...", "Restarting into recovery",
                                          "Rebooting to bootloader", "Entering download mode",
                                          "Restarting the device", "Shutting down"};

static void enter_download_mode(void) {
    /* On this platform there is no known userspace trigger for the MTK download
     * (ODIN) mode. The Download action now reboots with the reason "download"
     * via LINUX_REBOOT_CMD_RESTART2; whether the A22 5G bootloader acts on that
     * reason has not been validated on hardware.
     */
    jbm_log("jbm: requesting download mode (reboot reason 'download')");
}

static void act_system(char **argv, char **envp, const char *why);

static void perform(int idx, char **argv, char **envp) {
    switch (idx) {
        case ACT_SYSTEM:
            act_system(argv, envp, "user selected System");
            break;
        case ACT_RECOVERY:
            jbm_log("jbm: rebooting with reason 'recovery' (no flash write)");
            jbm_sync_all();
            jbm_sleep_ms(120);
            jbm_reboot_cmd(JB_RB_RECOVERY, 0);
            break;
        case ACT_FASTBOOT:
            jbm_log("jbm: rebooting to bootloader (fastboot)");
            jbm_sleep_ms(120);
            jbm_reboot_cmd(JB_RB_BOOTLOADER, 0);
            break;
        case ACT_DOWNLOAD:
            enter_download_mode();
            jbm_sleep_ms(120);
            jbm_reboot_cmd(JB_RB_DOWNLOAD, 0);
            break;
        case ACT_REBOOT:
            jbm_log("jbm: rebooting to system");
            jbm_sleep_ms(120);
            jbm_reboot_cmd(JB_RB_RESTART, 0);
            break;
        default:
            jbm_reboot_cmd(JB_RB_POWER_OFF, 0);
            break;
    }
    for (;;) jbm_sleep_ms(500);
}

/* ------------------------------------------------------------------ */
/* ui                                                                  */
/* ------------------------------------------------------------------ */

/* The two screens below follow the design mockups: a near black sky with a
 * large Jupiter on the right and a single column of options on the left, then
 * a "starting X" screen while the chosen action runs. Every size is given in
 * units of a 1080x2400 panel and scaled by L.k, so the layout follows the real
 * framebuffer instead of being hard-coded to one panel. */

struct lay {
    float k;
    int col_x, col_w;
    int item_h, item_gap, menu_top;
    int pcx, pcy, pr;
    int dirty_w;
};

static struct lay L;

static void layout_init(void) {
    float kh = (float)G.h / 2400.0f;
    float kw = (float)G.w / 1080.0f;
    float k = kh < kw ? kh : kw;
    if (k < 0.35f) k = 0.35f;
    L.k = k;
    L.col_x = (int)((float)G.w * 0.072f);
    L.col_w = (int)((float)G.w * 0.40f);
    L.item_h = (int)(86.0f * k);
    if (L.item_h < 20) L.item_h = 20;
    L.item_gap = (int)(11.0f * k);
    L.menu_top = (int)((float)G.h * 0.155f);
    L.pr = (int)((float)G.w * 0.34f);
    L.pcx = (int)((float)G.w * 0.86f);
    L.pcy = (int)((float)G.h * 0.55f);
    /* Everything that moves lives inside the column, so only that strip has to
     * be restored from the background and pushed to the framebuffer. It must
     * also stop before the planet, otherwise the two would overlap. */
    int dw = L.col_x + L.col_w + (int)(16.0f * k);
    int planet_left = L.pcx - L.pr - 2;
    if (dw > planet_left) dw = planet_left;
    if (dw > (int)G.w) dw = (int)G.w;
    if (dw < 1) dw = 1;
    L.dirty_w = dw;
}

struct ui {
    int sel;
    int running;
    i64 t0;
    i64 run_t;
    i64 countdown_ms;
    int countdown_on;
};

static struct ui U;

static i64 elapsed_ms(void) { return jbm_now_ms() - U.t0; }

static void item_geom(int i, int *x, int *y, int *w, int *h) {
    *x = L.col_x;
    *w = L.col_w;
    *h = L.item_h;
    *y = L.menu_top + i * (L.item_h + L.item_gap);
}

static i64 countdown_left(void) {
    if (!U.countdown_on) return 0;
    i64 el = jbm_now_ms() - U.t0 - JBM_INTRO_MS;
    i64 left = U.countdown_ms - el;
    return left < 0 ? 0 : left;
}

/* ------------------------------------------------------------------ */
/* background: sky, stars and Jupiter (painted once)                    */
/* ------------------------------------------------------------------ */

/* Band colours, dark belt to bright zone, sampled from the planet in the
 * design. */
static u32 PLR[256];

static void planet_ramp(void) {
    static const u16 stop_pos[] = {0, 48, 92, 132, 168, 198, 226, 255};
    static const u32 stop_col[] = {0x3B2822, 0x5A3A2B, 0x744D3A, 0x9A6B4B,
                                   0xC08A62, 0xD2A27C, 0xE4C7A4, 0xF2E3CC};
    for (int i = 0; i < 256; i++) {
        int s = 0;
        while (s < 6 && i > (int)stop_pos[s + 1]) s++;
        u32 span = (u32)(stop_pos[s + 1] - stop_pos[s]);
        u32 t = span ? (u32)(i - (int)stop_pos[s]) * 255u / span : 0;
        PLR[i] = mix(stop_col[s], stop_col[s + 1], t);
    }
}

static float jbm_cosf(float a) { return jbm_sinf(a + 1.5707963268f); }

static float jbm_sqrtf(float v) {
    if (v <= 0.0f) return 0.0f;
    float g = v * 0.5f + 1.0f;
    for (int i = 0; i < 14; i++) g = 0.5f * (g + v / g);
    return g;
}

static float smoothstepf(float e0, float e1, float x) {
    float t = (x - e0) / (e1 - e0);
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return t * t * (3.0f - 2.0f * t);
}

static float vhash(int x, int y) {
    u32 n = (u32)x * 0x9E3779B1u + (u32)y * 0x85EBCA77u;
    n ^= n >> 15;
    n *= 0x2545F491u;
    n ^= n >> 13;
    return (float)(n >> 8) * (1.0f / 16777216.0f);
}

static float vnoise(float x, float y) {
    int xi = (int)x, yi = (int)y;
    float fx = x - (float)xi, fy = y - (float)yi;
    fx = fx * fx * (3.0f - 2.0f * fx);
    fy = fy * fy * (3.0f - 2.0f * fy);
    float a = vhash(xi, yi), b = vhash(xi + 1, yi);
    float c = vhash(xi, yi + 1), d = vhash(xi + 1, yi + 1);
    float u = a + (b - a) * fx;
    float v = c + (d - c) * fx;
    return u + (v - u) * fy;
}

static u32 shade(u32 c, float f) {
    u32 r = (u32)((float)((c >> 16) & 0xFF) * f);
    u32 g = (u32)((float)((c >> 8) & 0xFF) * f);
    u32 b = (u32)((float)(c & 0xFF) * f);
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;
    return (r << 16) | (g << 8) | b;
}

/* Near black sky with a faint warm halo where the planet sits. */
static void sky_render(void) {
    float gr = (float)L.pr * 1.9f;
    float gx = (float)L.pcx, gy = (float)L.pcy;
    for (int y = 0; y < (int)G.h; y++) {
        float dy = ((float)y - gy) / gr;
        for (int x = 0; x < (int)G.w; x++) {
            float dx = ((float)x - gx) / gr;
            float f = 1.0f - jbm_sqrtf(dx * dx + dy * dy);
            if (f <= 0.0f) {
                store_px(x, y, 0xFF020202u);
                continue;
            }
            store_px(x, y, 0xFF000000u | mix(0x020202, 0x643E23, (u32)(f * f * 140.0f)));
        }
    }
}

static void stars_render(void) {
    u32 seed = 0x5EED1234u;
    i64 n = ((i64)G.w * (i64)G.h) / 9000;
    if (n > 420) n = 420;
    for (i64 i = 0; i < n; i++) {
        seed = seed * 1664525u + 1013904223u;
        int x = (int)((seed >> 8) % (u32)G.w);
        seed = seed * 1664525u + 1013904223u;
        int y = (int)((seed >> 8) % (u32)G.h);
        seed = seed * 1664525u + 1013904223u;
        int b = (int)((seed >> 8) % 100u);
        disc(x, y, b > 93 ? 2 : 1, (u32)(28 + (b % 6) * 24) << 24 | 0xFFFFFF);
    }
}

/* Jupiter: wavy bands from a colour ramp, a red spot over them and a sphere
 * lit from the upper left. */
static void planet_render(void) {
    int cx = L.pcx, cy = L.pcy, r = L.pr;
    float inv = 1.0f / (float)r;
    float sa = jbm_cosf(0.1745f), ss = jbm_sinf(0.1745f);
    for (int y = cy - r; y <= cy + r; y++) {
        if (y < 0 || y >= (int)G.h) continue;
        float ny = ((float)y - (float)cy) * inv;
        for (int x = cx - r; x <= cx + r; x++) {
            if (x < 0 || x >= (int)G.w) continue;
            float nx = ((float)x - (float)cx) * inv;
            float d2 = nx * nx + ny * ny;
            if (d2 > 1.0f) continue;
            float nz = jbm_sqrtf(1.0f - d2);

            float t1 = vnoise((float)x * 0.020f, (float)y * 0.055f);
            float t2 = vnoise((float)x * 0.075f, (float)y * 0.160f);
            float lat = ny + 0.085f * (t1 - 0.5f) + 0.030f * (t2 - 0.5f);
            float p = 0.50f + 0.30f * jbm_sinf(lat * 4.3f + 0.6f) +
                      0.155f * jbm_sinf(lat * 9.1f + 2.1f) +
                      0.080f * jbm_sinf(lat * 17.3f + 0.4f) +
                      0.045f * jbm_sinf(lat * 29.0f + 1.3f) +
                      0.022f * jbm_sinf(lat * 47.0f + 2.7f);
            if (p < 0.0f) p = 0.0f;
            if (p > 1.0f) p = 1.0f;
            u32 col = PLR[(int)(p * 255.0f)];

            float sx = nx - 0.26f, sy = ny - 0.17f;
            float u = (sx * sa + sy * ss) / 0.21f;
            float v = (-sx * ss + sy * sa) / 0.123f;
            float sd = jbm_sqrtf(u * u + v * v) + 0.16f * (t2 - 0.5f);
            if (sd < 1.45f) {
                u32 sc = mix(0x8C4830, 0x4B2118, (u32)((1.0f - smoothstepf(0.0f, 0.52f, sd)) * 255.0f));
                col = mix(col, sc, (u32)((1.0f - smoothstepf(0.55f, 1.42f, sd)) * 235.0f));
            }

            float diff = nx * -0.40f + ny * -0.46f + nz * 0.79f;
            if (diff < 0.0f) diff = 0.0f;
            u32 rgb = shade(col, 0.11f + 0.92f * diff);
            if (d2 > 0.90f && diff > 0.04f)
                rgb = mix(rgb, 0xE8C29A, (u32)(((d2 - 0.90f) / 0.10f) * diff * 140.0f));
            store_px(x, y, 0xFF000000u | rgb);
        }
    }
}

static void bg_build(void) {
    if (!G.ok || !G.cv || !G.bg) return;
    planet_ramp();
    sky_render();
    stars_render();
    planet_render();
    memcpy(G.bg, G.cv, G.stride * (u32)G.h);
}

/* Puts the static background back under the part of the canvas the menus
 * overdraw. */
static void restore_bg(void) {
    if (!G.bg) return;
    u32 bpr = G.w * (G.bpp / 8);
    u32 d = (u32)L.dirty_w * (G.bpp / 8);
    if (d > bpr) d = bpr;
    for (u32 y = 0; y < G.h; y++)
        memcpy(G.cv + (u32)y * G.stride, G.bg + (u32)y * G.stride, d);
}

/* ------------------------------------------------------------------ */
/* widgets                                                             */
/* ------------------------------------------------------------------ */

/* Rounded rectangle filled with a horizontal alpha ramp. */
static void round_rect_grad(int x, int y, int w, int h, int r, u32 rgb, int a0, int a1) {
    if (w <= 0 || h <= 0) return;
    if (r > w / 2) r = w / 2;
    if (r > h / 2) r = h / 2;
    if (r < 0) r = 0;
    rgb &= 0xFFFFFF;
    for (int row = 0; row < h; row++) {
        int e = row < h - 1 - row ? row : h - 1 - row;
        int in = (r > 0 && e < r) ? rr_inset(r, e) : 0;
        int x0 = x + in, x1 = x + w - in;
        int span = x1 - x0 - 1;
        if (span <= 0) continue;
        for (int cx = x0; cx < x1; cx++) {
            int a = a0 + (a1 - a0) * (cx - x0) / span;
            if (a > 0) blend(cx, y + row, ((u32)a << 24) | rgb);
        }
    }
}

/* Tilted ellipse outline, used for the ring around the brand mark. */
static void ellipse_ring(int cx, int cy, int rx, int ry, int th, float ang, u32 c) {
    if (rx <= 0 || ry <= 0 || th <= 0) return;
    float ca = jbm_cosf(ang), sa = jbm_sinf(ang);
    float minr = rx < ry ? (float)rx : (float)ry;
    int ext = (rx > ry ? rx : ry) + th;
    for (int y = cy - ext; y <= cy + ext; y++) {
        for (int x = cx - ext; x <= cx + ext; x++) {
            float dx = (float)x - (float)cx, dy = (float)y - (float)cy;
            float u = (dx * ca + dy * sa) / (float)rx;
            float v = (-dx * sa + dy * ca) / (float)ry;
            float d = (jbm_sqrtf(u * u + v * v) - 1.0f) * minr;
            if (d < 0.0f) d = -d;
            if (d <= (float)th * 0.5f) blend(x, y, c);
        }
    }
}

/* The planet mark next to the wordmark. Returns the x the word starts at. */
static int draw_brand(int cx, int cy, int r, float name_px, u32 icon_c, u32 accent, int dot) {
    int th = (int)((float)r * 0.11f) + 1;
    ring(cx, cy, r, th, icon_c);
    ellipse_ring(cx, cy, (int)((float)r * 1.16f), (int)((float)r * 0.32f), th, -0.3142f, icon_c);
    if (dot)
        disc(cx + r + (int)((float)r * 0.12f), cy - r + (int)((float)r * 0.22f),
             (int)((float)r * 0.14f) + 1, icon_c);
    int tx = cx + r + (int)((float)r * 0.42f);
    draw_pair(&FN32, tx, cy, name_px, TA_L | VA_M, (int)(name_px * 0.04f), RGBA(0xEEEEEEFF), "Jupiter",
              accent, "Boot");
    return tx;
}

/* ------------------------------------------------------------------ */
/* screen 1: the option list                                           */
/* ------------------------------------------------------------------ */

static void draw_menu(void) {
    float k = L.k;
    i64 el = elapsed_ms();
    /* Entry animation: the column slides up and fades in, item by item. */
    int left = el < JBM_INTRO_MS ? JBM_INTRO_MS - (int)el : 0;
    int ey = left * (int)((float)G.h * 0.012f) / JBM_INTRO_MS;

    int br = (int)(28.0f * k);
    int top = (int)((float)G.h * 0.075f);
    int tx = draw_brand(L.col_x + br, top + br + ey, br, 44.0f * k, RGBA(0xBD7946FF),
                        RGBA(0xC47B48FF), 0);
    char dev[40];
    jbm_sn(dev, "JBM v%s", JBM_VERSION);
    draw_text_ls(&FN20, tx, top + (int)(84.0f * k) + ey, 24.0f * k, RGBA(0x777777FF), dev, TA_L | VA_M,
                 (int)(4.0f * k));

    for (int i = 0; i < ACT_COUNT; i++) {
        int x, y, w, h;
        item_geom(i, &x, &y, &w, &h);
        y += ey;
        i64 start = 40 + i * 40;
        i64 a = el - start;
        if (a < 0) continue;
        int al = (int)(a < 320 ? (float)a * 255.0f / 320.0f : 255.0f);
        x += (255 - al) * (int)((float)G.w * 0.05f) / 255;
        int sel = (i == U.sel);
        int r = (int)(6.0f * k) + 1;

        if (sel) {
            round_rect_grad(x, y, w, h, r, 0xB86837, 56, 6);
            round_rect_border(x, y, w, h, r, (int)(1.0f * k) + 1, RGBA(0xD88A4E40));
            int lw = (int)(2.0f * k) + 1;
            round_rect(x, y, lw, h, r, RGBA(0xD88A4EFF));
            int bar = (int)(2.0f * k) + 1;
            int inset = (int)(9.0f * k);
            fill_rect(x - bar * 3, y + inset, bar * 3, h - inset * 2, RGBA(0xDF925530));
            fill_rect(x - bar, y + inset, bar, h - inset * 2, RGBA(0xDF9255FF));
        }

        u32 fg = sel ? RGBA(0xFFFFFFFF) : RGBA(0x858585FF);
        fg = (fg & 0x00FFFFFFu) | ((u32)al << 24);
        draw_text_ls(&FN32, x + (int)((float)w * 0.115f), y + h / 2, 33.0f * k, fg, act_label_up[i],
                     TA_L | VA_M, (int)(4.0f * k));
    }

    int menu_bottom = L.menu_top + ACT_COUNT * (L.item_h + L.item_gap) - L.item_gap;
    int fy = menu_bottom + (int)(38.0f * k) + ey;
    char foot[64];
    jbm_sn(foot, "TOUCH TO SELECT \xB7 %s", act_label_up[U.sel]);
    draw_text_ls(&FN20, L.col_x, fy, 22.0f * k, RGBA(0x4E4E4EFF), foot, TA_L | VA_M, (int)(3.0f * k));

    if (U.countdown_on) {
        i64 left_s = countdown_left();
        i64 secs = (left_s + 999) / 1000;
        char num[8];
        num[0] = (char)('0' + (int)(secs / 10) % 10);
        num[1] = (char)('0' + (int)secs % 10);
        num[2] = 0;
        int cy = fy + (int)(66.0f * k);
        draw_text_ls(&FN20, L.col_x, cy, 20.0f * k, RGBA(0x3C3C3CFF), "AUTO-BOOT IN", TA_L | VA_T,
                     (int)(3.0f * k));
        draw_text(&FDIG, L.col_x, cy + (int)(44.0f * k), 96.0f * k, RGBA(0xC47B4880), num, TA_L | VA_T);
        int by = cy + (int)(176.0f * k);
        int th = (int)(3.0f * k) + 1;
        fill_rect(L.col_x, by, L.col_w, th, RGBA(0x141414FF));
        i64 total = U.countdown_ms > 0 ? U.countdown_ms : 1;
        i64 prog = total - left_s;
        if (prog < 0) prog = 0;
        if (prog > total) prog = total;
        fill_rect(L.col_x, by, (int)((i64)L.col_w * prog / total), th, RGBA(0xC47B4866));
    } else {
        draw_text_ls(&FN20, L.col_x, fy + (int)(66.0f * k), 20.0f * k, RGBA(0x3C3C3CFF),
                     "AUTO-BOOT CANCELLED", TA_L | VA_T, (int)(3.0f * k));
    }
}

/* ------------------------------------------------------------------ */
/* screen 2: the chosen action starting                                */
/* ------------------------------------------------------------------ */

static void draw_running(int idx) {
    float k = L.k;
    int br = (int)(32.0f * k);
    int top = (int)((float)G.h * 0.075f);
    int tx = draw_brand(L.col_x + br, top + br, br, 46.0f * k, RGBA(0xC87335FF), RGBA(0xC87335FF), 1);
    char dev[40];
    jbm_sn(dev, "JBM v%s", JBM_VERSION);
    draw_text_ls(&FN20, tx, top + (int)(96.0f * k), 26.0f * k, RGBA(0x999999FF), dev, TA_L | VA_M,
                 (int)(5.0f * k));

    int y = (int)((float)G.h * 0.285f);
    draw_text_ls(&FN32, L.col_x, y, 26.0f * k, RGBA(0xDDDDDDFF), "STARTING", TA_L | VA_T,
                 (int)(10.0f * k));
    y += (int)(58.0f * k);
    draw_text_ls(&FN48, L.col_x, y, 104.0f * k, RGBA(0xC87335FF), act_label_up[idx], TA_L | VA_T,
                 (int)(18.0f * k));
    y += (int)(158.0f * k);
    draw_text_ls(&FN32, L.col_x, y, 34.0f * k, RGBA(0xDDDDDDFF), act_note[idx], TA_L | VA_T,
                 (int)(7.0f * k));

    /* Spinner: a dim full circle with a bright half sweeping clockwise. */
    int sr = (int)(36.0f * k);
    int th = (int)(6.0f * k) + 1;
    int scx = L.col_x + sr, scy = y + (int)(104.0f * k);
    ring(scx, scy, sr, th, RGBA(0xFFFFFF1A));
    int ang = -(int)((jbm_now_ms() % 1150) * 360 / 1150);
    arc(scx, scy, sr, th + (int)(2.0f * k), ang, ang + 180, RGBA(0xC87335FF));

    int fy = (int)((float)G.h * 0.925f);
    int ls = (int)(6.0f * k);
    draw_text_ls(&FN20, L.col_x, fy, 24.0f * k, RGBA(0x8B542FFF), "JUPITERBOOT", TA_L | VA_M, ls);
    char foot[64];
    jbm_sn(foot, " \xB7 %s", act_label_up[idx]);
    draw_text_ls(&FN20, L.col_x + measure_ls(&FN20, "JUPITERBOOT", 24.0f * k, ls), fy, 24.0f * k,
                 RGBA(0x666666FF), foot, TA_L | VA_M, ls);
}

static void draw_ui(void) {
    if (!G.ok) return;
    restore_bg();
    if (U.running >= 0) draw_running(U.running);
    else draw_menu();
}

static int hit_item(int px, int py) {
    for (int i = 0; i < ACT_COUNT; i++) {
        int x, y, w, h;
        item_geom(i, &x, &y, &w, &h);
        if (px >= x && px < x + w && py >= y && py < y + h) return i;
    }
    return -1;
}

static void present(void) {
    if (!G.ok) return;
    u32 bpr = G.w * (G.bpp / 8);
    u32 d = (u32)L.dirty_w * (G.bpp / 8);
    if (d > bpr) d = bpr;
    for (u32 y = 0; y < G.h; y++)
        memcpy(G.mem + (u32)y * G.stride, G.cv + (u32)y * G.stride, d);
}

/* First frame goes out whole, so the frozen boot logo under the planet is
 * replaced too. */
static void present_full(void) {
    if (!G.ok) return;
    memcpy(G.mem, G.cv, G.stride * (u32)G.h);
}

/* ------------------------------------------------------------------ */
/* entry                                                               */
/* ------------------------------------------------------------------ */

static void cleanup_and_chain(char **argv, char **envp, const char *why) {
    jbm_log("jbm: chaining to %s (%s)", JBM_REAL_INIT, why);
    if (G.ok && G.mem && G.cv) present();
#ifndef JBM_HOST
    if (G.mem) {
        sc2(SYS_munmap, (long)G.mem, G.len);
        G.mem = 0;
    }
    if (G.cv) {
        sc2(SYS_munmap, (long)G.cv, G.stride * (u32)G.h);
        G.cv = 0;
    }
    if (G.bg) {
        sc2(SYS_munmap, (long)G.bg, G.stride * (u32)G.h);
        G.bg = 0;
    }
#endif
    /* Close the menu's fds so they do not leak into /init.system and so
     * umount("/dev") is not EBUSY. */
    if (G.ok && G.fd >= 0) jbm_close(G.fd);
    G.fd = -1;
    if (T.fd >= 0) jbm_close(T.fd);
    T.fd = -1;
    if (jbm_klog_fd >= 0) jbm_close(jbm_klog_fd);
    jbm_klog_fd = -1;
    /* TODO(hardware): unmounting /dev before execve may break the Android
     * first-stage init, which can expect /dev to stay populated. The fds above
     * are now closed, so the umount should no longer fail with EBUSY. Needs
     * testing on the device; consider leaving /dev mounted. */
    jbm_umount("/sys");
    jbm_umount("/proc");
    jbm_umount("/dev");
    jbm_sleep_ms(20);
#ifdef JBM_HOST
    jbm_log("jbm: host build, would exec %s", JBM_REAL_INIT);
#else
    sc6(SYS_execve, (long)JBM_REAL_INIT, (long)argv, (long)envp, 0, 0, 0);
    jbm_log("jbm: exec %s failed, restarting instead of re-running the menu", JBM_REAL_INIT);
    jbm_sleep_ms(200);
    jbm_reboot_cmd(JB_RB_RESTART, 0);
#endif
    for (;;) jbm_sleep_ms(200);
}

static void act_system(char **argv, char **envp, const char *why) { cleanup_and_chain(argv, envp, why); }

int jbm_main(int argc, char **argv, char **envp);

/* Sets up just enough of /dev to log, read input and use the framebuffer. */
static void dev_init(void) {
#ifndef JBM_HOST
    jbm_log("jbm: dev_init begin");
    jbm_mkdir("/dev", 0755);
    i64 r = jbm_mount("tmpfs", "/dev", "tmpfs", 0, NULL);
    if (r != 0) jbm_log("jbm: tmpfs on /dev failed (%d), reusing ramdisk /dev", (int)r);
    i64 c = jbm_mknod("/dev/console", 0600 | S_IFCHR, (5 << 8) | 1);
    i64 k = jbm_mknod("/dev/kmsg", 0600 | S_IFCHR, (1 << 8) | 11);
    i64 nu = jbm_mknod("/dev/null", 0666 | S_IFCHR, (1 << 8) | 3);
    jbm_log("jbm: dev_init mount=%d mknod console=%d kmsg=%d null=%d", (int)r, (int)c, (int)k,
            (int)nu);
#endif
    /* -1 leaves jbm_log() free to retry later if the console node was not
     * available yet when dev_init() ran. */
    jbm_klog_fd = -1;
    jbm_log("jbm: dev_init done, klog_fd=%d kmsg_err=%d con_err=%d", jbm_klog_fd, jbm_kmsg_err,
            jbm_con_err);
}

#ifdef JBM_HOST
/* Host preview: renders the real draw_ui() path into an off-screen ARGB8888
 * canvas and writes a binary PPM, so layout and colours can be checked without
 * a device. Usage: jbm_host <out.ppm> [at_ms] [menu|system|recovery|fastboot|download|reboot|"power off"]
 */
static int jbm_str_ieq(const char *a, const char *b) {
    for (; *a && *b; a++, b++) {
        char x = *a, y = *b;
        if (x >= 'A' && x <= 'Z') x = (char)(x + 32);
        if (y >= 'A' && y <= 'Z') y = (char)(y + 32);
        if (x != y) return 0;
    }
    return *a == *b;
}

static void host_preview(const char *path, i64 at_ms, int running) {
    G.w = 1080;
    G.h = 2400;
    G.bpp = 32;
    G.stride = G.w * 4;
    G.len = G.stride * G.h;
    G.rl = G.gl = G.bl = G.tl = 8;
    G.ro = 16;
    G.go = 8;
    G.bo = 0;
    G.to = 24;
    G.cv = (u8 *)calloc(1, G.len);
    G.bg = (u8 *)calloc(1, G.len);
    G.mem = (u8 *)calloc(1, G.len);
    if (!G.cv || !G.mem || !G.bg) {
        fprintf(stderr, "[jbm:host] out of memory\n");
        exit(1);
    }
    G.ok = 1;

    layout_init();
    bg_build();

    U.sel = running >= 0 ? running : ACT_SYSTEM;
    U.running = running;
    U.run_t = jbm_now_ms();
    U.t0 = jbm_now_ms() - at_ms;
    U.countdown_ms = JBM_AUTOBOOT_SEC * 1000;
    U.countdown_on = JBM_AUTOBOOT_SEC > 0;

    draw_ui();

    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "[jbm:host] cannot open %s\n", path);
        exit(1);
    }
    fprintf(f, "P6\n%u %u\n255\n", G.w, G.h);
    for (u32 y = 0; y < G.h; y++) {
        for (u32 x = 0; x < G.w; x++) {
            u32 c = load_px((int)x, (int)y);
            fputc((int)((c >> 16) & 0xFF), f);
            fputc((int)((c >> 8) & 0xFF), f);
            fputc((int)(c & 0xFF), f);
        }
    }
    fclose(f);
    fprintf(stderr, "[jbm:host] wrote %s (%ux%u, %s, countdown %llds left)\n", path, G.w, G.h,
            running >= 0 ? act_label[running] : "menu", (long long)((countdown_left() + 999) / 1000));
}

int main(int argc, char **argv) {
    fprintf(stderr, "[jbm:host] argc=%d argv0=%s\n", argc, argc > 0 ? argv[0] : "-");
    dev_init();
    if (argc >= 2) {
        i64 at_ms = argc >= 3 ? (i64)atoi(argv[2]) : 0;
        const char *screen = argc >= 4 ? argv[3] : "menu";
        int running = -1;
        for (int i = 0; i < ACT_COUNT; i++)
            if (jbm_str_ieq(screen, act_label[i]) || jbm_str_ieq(screen, act_label_up[i])) {
                running = i;
                break;
            }
        host_preview(argv[1], at_ms, running);
        return 0;
    }
    touch_open();
    fprintf(stderr, "[jbm:host] gfx_ok=%d touch_fd=%d\n", G.ok, T.fd);
    fprintf(stderr,
            "[jbm:host] usage: %s out.ppm [at_ms] [menu|system|recovery|fastboot|download|reboot|poweroff]\n",
            argv[0]);
    return 0;
}
#endif

int jbm_main(int argc, char **argv, char **envp) {
    dev_init();
    if (jbm_mount("proc", "/proc", "proc", 0, NULL) != 0)
        jbm_log("jbm: mount /proc failed");
    if (jbm_mount("sysfs", "/sys", "sysfs", 0, NULL) != 0)
        jbm_log("jbm: mount /sys failed");
    jbm_log("jbm: v%s start argc=%d argv0=%s", JBM_VERSION, argc, argc > 0 && argv ? argv[0] : "-");

    /* The display driver may register fb0 a few seconds after init starts, so
     * poll for a while instead of giving up after 3 s. */
    int fb = 0;
    int last_err = -1;
    i64 t_first = jbm_now_ms();
    for (int i = 0; i < 400; i++) {
        gfx_quiet = (i != 0);
        if (gfx_open() == 0) {
            fb = 1;
            jbm_log("jbm: fb ready after %lld ms (attempt %d)", (long long)(jbm_now_ms() - t_first), i);
            break;
        }
        if (gfx_err != last_err) {
            jbm_log("jbm: fb attempt %d failed (err %d)", i, gfx_err);
            last_err = gfx_err;
        } else if (i % 40 == 39) {
            jbm_log("jbm: fb still unavailable after %lld ms (err %d)",
                    (long long)(jbm_now_ms() - t_first), gfx_err);
        }
        jbm_sleep_ms(50);
    }
    gfx_quiet = 0;
    if (!fb) {
        jbm_log("jbm: framebuffer unavailable after %lld ms, booting Android directly",
                (long long)(jbm_now_ms() - t_first));
        cleanup_and_chain(argv, envp, "no framebuffer");
    }

    touch_open();

    layout_init();
    bg_build();

    U.sel = ACT_SYSTEM;
    U.running = -1;
    U.t0 = jbm_now_ms();
    U.countdown_ms = JBM_AUTOBOOT_SEC * 1000;
    U.countdown_on = JBM_AUTOBOOT_SEC > 0;

    i64 start = U.t0;
    i64 last_draw = 0;
    int kicked = 0;
    i64 kick_t = 0;
    int first = 1;

    for (;;) {
        i64 now = jbm_now_ms();
        touch_poll();

        /* Any touch stops the auto-start: the device waits for a decision. */
        if (T.pressed) {
            U.countdown_on = 0;
            int idx = hit_item(T.sx, T.sy);
            if (idx >= 0) U.sel = idx;
        }

        /* One tap is enough for every mode, System included. */
        if (T.released) {
            U.countdown_on = 0;
            int idx = hit_item(T.tap_x, T.tap_y);
            if (idx >= 0 && U.running < 0) {
                U.sel = idx;
                U.running = idx;
                U.run_t = now;
                jbm_log("jbm: action %s tapped", act_label[idx]);
            }
        }

        /* The "starting X" screen stays up long enough to be seen. */
        if (U.running >= 0 && now - U.run_t >= JBM_RUN_MS) {
            int go = U.running;
            U.running = -1;
            perform(go, argv, envp);
        }

        if (U.running < 0 && U.countdown_on && elapsed_ms() >= JBM_INTRO_MS &&
            countdown_left() <= 0) {
            U.sel = ACT_SYSTEM;
            U.running = ACT_SYSTEM;
            U.run_t = now;
            jbm_log("jbm: countdown elapsed, starting System");
        }

        if (U.running < 0 && now - start > (i64)JBM_TOTAL_TIMEOUT_SEC * 1000) {
            cleanup_and_chain(argv, envp, "safety timeout");
        }

        if (now - last_draw > 33) {
            draw_ui();
            if (first) {
                first = 0;
                present_full();
            } else {
                present();
            }
            last_draw = now;
            /* Two kicks: right after the first frame (so the layer is
             * reprogrammed with the menu already in memory) and once more
             * later, in case something re-asserts the boot logo. */
            if (kicked == 0) {
                kicked = 1;
                kick_t = now;
                gfx_kick();
                first = 1; /* the kick can blank the panel: repaint it all */
            } else if (kicked == 1 && now - kick_t > 1500) {
                kicked = 2;
                gfx_kick();
                first = 1;
            }
        } else {
            jbm_sleep_ms(8);
        }
    }
    return 0;
}
