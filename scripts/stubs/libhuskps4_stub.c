// SPDX-License-Identifier: GPL-2.0-or-later
// libhuskps4.dylib stub for CI builds.
//
// The real libhuskps4.dylib is built by scripts/build_ps4_ios.sh from two
// private source trees (Bachata-S4 on branch husk, husk-fex on branch
// husk-fex) that are not part of this repository. Without them the dylib
// cannot be built in CI. project.yml embeds the dylib unconditionally
// (embed: true) and xcodebuild's "Embed Frameworks" phase fails the build
// when the file is missing, so a CI build dies at the link step.
//
// This stub is the fallback: it exports the one symbol the host expects
// (husk_console_module_v1) and returns a module table whose every entry
// reports failure. PS4Store.moduleURL (which only checks file existence)
// will report the module as present, but the host's dlopen+dlsym path in
// husk-console-host.c will load this stub, get a table whose init() fails,
// and surface a clear error to the player. Everything else in Husk --
// the translation layer, the QEMU/LineageOS guest, the A13 Android path --
// is unaffected. The stub is only built when the real dylib is absent,
// so local builds that have the private trees are not touched.
#include "../../src/translation-layer-next/husk-console-abi.h"

#include <string.h>

static const char kUnavailable[] =
    "PS4 layer not available in this build (libhuskps4 stub: the private "
    "Bachata-S4 / husk-fex sources were not present at build time).";

static int stub_init(const HuskHostV1 *host, const char *user_dir) {
    (void)host; (void)user_dir;
    if (host && host->log_line) host->log_line(kUnavailable);
    return 1;  // nonzero = failure, per the ABI
}

static int stub_launch(const char *game, const char *args, void *metal_layer,
                       int width_px, int height_px, float scale) {
    (void)game; (void)args; (void)metal_layer;
    (void)width_px; (void)height_px; (void)scale;
    return 1;  // nonzero = launch did not begin
}

static int stub_state(void) { return HUSK_CONSOLE_FAILED; }
static uint64_t stub_frames(void) { return 0; }
static void stub_set_paused(int p) { (void)p; }
static void stub_set_pad(const HuskPadV1 *pad) { (void)pad; }
static void stub_resize(int w, int h) { (void)w; (void)h; }
static void stub_stop(void) {}

static int stub_take_pending_restart(char *path, int path_size,
                                      char *args, int args_size) {
    (void)path; (void)path_size; (void)args; (void)args_size;
    return 0;  // no pending restart
}

static int stub_install_package(const char *package, const char *dest_dir,
                                void (*progress)(void *, uint64_t, uint64_t),
                                void *ctx, char *err, int err_size) {
    (void)package; (void)dest_dir; (void)progress; (void)ctx;
    if (err && err_size > 0) {
        // Truncate to fit, always NUL-terminated.
        size_t n = sizeof(kUnavailable) - 1;
        if (n > (size_t)err_size - 1) n = (size_t)err_size - 1;
        memcpy(err, kUnavailable, n);
        err[n] = '\0';
    }
    return 1;  // nonzero = not extracted
}

static const HuskConsoleModuleV1 kModule = {
    .struct_size = sizeof(HuskConsoleModuleV1),
    .version = HUSK_CONSOLE_ABI_VERSION,
    .platform = "ps4",
    .jit_ceiling_bytes = 0,  // asks for nothing; the host's region is always big enough
    .init = stub_init,
    .launch = stub_launch,
    .state = stub_state,
    .frames = stub_frames,
    .set_paused = stub_set_paused,
    .set_pad = stub_set_pad,
    .resize = stub_resize,
    .stop = stub_stop,
    .take_pending_restart = stub_take_pending_restart,
    .install_package = stub_install_package,
};

const HuskConsoleModuleV1 *husk_console_module_v1(void) {
    return &kModule;
}
