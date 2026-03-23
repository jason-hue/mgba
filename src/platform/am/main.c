#include <am.h>
#include <klib.h>

#include <mgba/core/core.h>
#include <mgba/gba/core.h>
#include <mgba/internal/gba/input.h>
#include <mgba-util/vfs.h>

#include "roms.h"

#define FB_W 240
#define FB_H 160

static struct mCore* core;
static mColor framebuffer[FB_W * FB_H];
static bool pressed[256];
static int draw_x;
static int draw_y;
static bool running;
static bool framebuffer_reported;

static void am_init_video(void) {
	AM_GPU_CONFIG_T cfg;

	ioe_read(AM_GPU_CONFIG, &cfg);
	draw_x = cfg.width > FB_W ? (cfg.width - FB_W) / 2 : 0;
	draw_y = cfg.height > FB_H ? (cfg.height - FB_H) / 2 : 0;
	memset(framebuffer, 0, sizeof(framebuffer));
}

static void am_flush_video(void) {
	AM_GPU_FBDRAW_T draw = {
		.x = draw_x,
		.y = draw_y,
		.pixels = framebuffer,
		.w = FB_W,
		.h = FB_H,
		.sync = true,
	};

	ioe_write(AM_GPU_FBDRAW, &draw);
}

static bool am_framebuffer_active(void) {
	size_t i;

	for (i = 0; i < sizeof(framebuffer) / sizeof(framebuffer[0]); ++i) {
		if (framebuffer[i] != 0) {
			return true;
		}
	}

	return false;
}

static void am_poll_input(void) {
	AM_INPUT_KEYBRD_T ev;

	do {
		ioe_read(AM_INPUT_KEYBRD, &ev);
		if (ev.keycode == AM_KEY_NONE) {
			break;
		}
		assert(ev.keycode >= 0 && ev.keycode < (int)(sizeof(pressed) / sizeof(pressed[0])));
		pressed[ev.keycode] = ev.keydown;
		if (ev.keydown && (ev.keycode == AM_KEY_ESCAPE || ev.keycode == AM_KEY_Q)) {
			running = false;
		}
	} while (1);
}

static uint32_t am_build_gba_keys(void) {
	uint32_t keys = 0;

	if (pressed[AM_KEY_X]) keys |= 1U << GBA_KEY_A;
	if (pressed[AM_KEY_Z]) keys |= 1U << GBA_KEY_B;
	if (pressed[AM_KEY_RETURN]) keys |= 1U << GBA_KEY_START;
	if (pressed[AM_KEY_BACKSPACE]) keys |= 1U << GBA_KEY_SELECT;
	if (pressed[AM_KEY_UP]) keys |= 1U << GBA_KEY_UP;
	if (pressed[AM_KEY_DOWN]) keys |= 1U << GBA_KEY_DOWN;
	if (pressed[AM_KEY_LEFT]) keys |= 1U << GBA_KEY_LEFT;
	if (pressed[AM_KEY_RIGHT]) keys |= 1U << GBA_KEY_RIGHT;
	if (pressed[AM_KEY_S]) keys |= 1U << GBA_KEY_R;
	if (pressed[AM_KEY_A]) keys |= 1U << GBA_KEY_L;

	return keys;
}

static const struct embedded_rom* am_select_rom(const char* args) {
	int i;

	if (nroms <= 0) {
		return NULL;
	}

	if (!args || !*args) {
		return &roms[0];
	}

	for (i = 0; i < nroms; ++i) {
		if (strcmp(args, roms[i].name) == 0) {
			return &roms[i];
		}
	}

	return &roms[0];
}

static bool am_load_rom(const char* args) {
	const struct embedded_rom* rom = am_select_rom(args);
	struct VFile* vf;

	if (!rom) {
		printf("No embedded ROMs found.\n");
		return false;
	}

	printf("Loading ROM: %s\n", rom->name);
	vf = VFileFromConstMemory(rom->data, rom->size);
	if (!vf) {
		printf("Failed to create ROM VFile.\n");
		return false;
	}

	if (!core->loadROM(core, vf)) {
		printf("Failed to load ROM: %s\n", rom->name);
		return false;
	}

	return true;
}

int main(const char* args) {
	ioe_init();
	am_init_video();
	running = true;
	framebuffer_reported = false;

	core = GBACoreCreate();
	assert(core);
	assert(core->init(core));
	mCoreInitConfig(core, "am");
	core->setAudioBufferSize(core, 2048);
	core->setVideoBuffer(core, framebuffer, FB_W);

	if (!am_load_rom(args)) {
		core->deinit(core);
		return 1;
	}

	core->reset(core);

	while (running) {
		uint32_t keys;
		am_poll_input();
		keys = am_build_gba_keys();
		core->setKeys(core, keys);
		core->runFrame(core);
		if (!framebuffer_reported && am_framebuffer_active()) {
			printf("Framebuffer received non-zero pixels.\n");
			framebuffer_reported = true;
		}
		am_flush_video();
	}

	core->deinit(core);
	return 0;
}
