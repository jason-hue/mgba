#include <am.h>
#include <klib.h>

#include <mgba/core/log.h>
#include <mgba/core/core.h>
#include <mgba/gba/core.h>
#include <mgba/internal/gba/input.h>
#include <mgba-util/audio-buffer.h>
#include <mgba-util/vfs.h>

#include "roms.h"

#define FB_W 240
#define FB_H 160
#define AUDIO_CHANNELS 2
#define AUDIO_SAMPLES 1024
#define AUDIO_CHUNK_FRAMES 1024

static struct mCore* core;
static struct mStandardLogger logger;
static mColor framebuffer[FB_W * FB_H];
static uint32_t am_framebuffer[FB_W * FB_H];
static int16_t audio_chunk[AUDIO_CHUNK_FRAMES * AUDIO_CHANNELS];
static bool pressed[256];
static int draw_x;
static int draw_y;
static bool running;
static bool audio_enabled;
static int audio_bufsize;
static uint64_t frame_time_us;
static uint64_t next_frame_deadline_us;
static uint64_t fps_window_start_us;
static uint32_t fps_window_frames;
static bool fps_line_active;

static void am_poll_input(void);

static void am_deinit_audio(void) {
	audio_enabled = false;
	audio_bufsize = 0;
}

static uint64_t am_uptime_us(void) {
	AM_TIMER_UPTIME_T uptime;

	ioe_read(AM_TIMER_UPTIME, &uptime);
	return uptime.us;
}

static void am_handle_key_event(const AM_INPUT_KEYBRD_T* ev) {
	if (ev->keycode == AM_KEY_NONE) {
		return;
	}

	assert(ev->keycode >= 0 && ev->keycode < (int)(sizeof(pressed) / sizeof(pressed[0])));
	pressed[ev->keycode] = ev->keydown;
	if (ev->keydown && (ev->keycode == AM_KEY_ESCAPE || ev->keycode == AM_KEY_Q)) {
		running = false;
	}
}

static void am_init_video(void) {
	AM_GPU_CONFIG_T cfg;

	ioe_read(AM_GPU_CONFIG, &cfg);
	draw_x = cfg.width > FB_W ? (cfg.width - FB_W) / 2 : 0;
	draw_y = cfg.height > FB_H ? (cfg.height - FB_H) / 2 : 0;
	memset(framebuffer, 0, sizeof(framebuffer));
	memset(am_framebuffer, 0, sizeof(am_framebuffer));
}

static void am_flush_video(void) {
	size_t i;
	for (i = 0; i < FB_W * FB_H; ++i) {
		uint32_t pixel = framebuffer[i];
		am_framebuffer[i] = (pixel & 0x0000FF00) | ((pixel & 0x000000FF) << 16) | ((pixel & 0x00FF0000) >> 16);
	}

	AM_GPU_FBDRAW_T draw = {
		.x = draw_x,
		.y = draw_y,
		.pixels = am_framebuffer,
		.w = FB_W,
		.h = FB_H,
		.sync = true,
	};

	ioe_write(AM_GPU_FBDRAW, &draw);
}

static void am_init_timing(void) {
	uint64_t frame_cycles = core->frameCycles(core);
	uint64_t frequency = core->frequency(core);

	if (!frame_cycles || !frequency) {
		frame_time_us = 16667;
	} else {
		frame_time_us = (frame_cycles * 1000000ULL + frequency - 1) / frequency;
	}
	next_frame_deadline_us = am_uptime_us();
}

static void am_throttle_frame(void) {
	uint64_t now;

	if (!frame_time_us) {
		return;
	}

	next_frame_deadline_us += frame_time_us;
	now = am_uptime_us();
	if (now > next_frame_deadline_us + frame_time_us) {
		next_frame_deadline_us = now;
		return;
	}

	while (running && (now = am_uptime_us()) < next_frame_deadline_us) {
		am_poll_input();
	}
}

static void am_report_fps(void) {
	uint64_t now = am_uptime_us();
	uint64_t elapsed_us;

	if (!fps_window_start_us) {
		fps_window_start_us = now;
	}

	++fps_window_frames;
	elapsed_us = now - fps_window_start_us;
	if (elapsed_us < 1000000ULL) {
		return;
	}

	printf("\rFPS: %u   ", (unsigned) ((fps_window_frames * 1000000ULL + elapsed_us / 2) / elapsed_us));
	fflush(stdout);
	fps_line_active = true;
	fps_window_start_us = now;
	fps_window_frames = 0;
}

static void am_init_audio(void) {
	AM_AUDIO_CONFIG_T cfg;
	AM_AUDIO_CTRL_T ctrl;
	unsigned sample_rate;

	am_deinit_audio();
	ioe_read(AM_AUDIO_CONFIG, &cfg);
	if (!cfg.present) {
		return;
	}
	audio_bufsize = cfg.bufsize;

	sample_rate = core->audioSampleRate(core);
	if (!sample_rate || audio_bufsize <= 0) {
		return;
	}

	ctrl.freq = sample_rate;
	ctrl.channels = AUDIO_CHANNELS;
	ctrl.samples = AUDIO_SAMPLES;
	ioe_write(AM_AUDIO_CTRL, &ctrl);
	audio_enabled = true;
}

static void am_flush_audio(void) {
	struct mAudioBuffer* buffer;
	AM_AUDIO_STATUS_T status;
	size_t frames;
	size_t free_frames;

	if (!audio_enabled) {
		return;
	}

	buffer = core->getAudioBuffer(core);
	if (!buffer) {
		return;
	}

	frames = mAudioBufferAvailable(buffer);
	if (!frames) {
		return;
	}

	ioe_read(AM_AUDIO_STATUS, &status);
	if (status.count >= audio_bufsize) {
		return;
	}

	free_frames = (audio_bufsize - status.count) / (AUDIO_CHANNELS * sizeof(audio_chunk[0]));
	if (!free_frames) {
		return;
	}
	if (frames > AUDIO_CHUNK_FRAMES) {
		frames = AUDIO_CHUNK_FRAMES;
	}
	if (frames > free_frames) {
		frames = free_frames;
	}
	frames = mAudioBufferRead(buffer, audio_chunk, frames);
	if (!frames) {
		return;
	}

	AM_AUDIO_PLAY_T play;
	play.buf.start = audio_chunk;
	play.buf.end = (uint8_t*) audio_chunk + frames * AUDIO_CHANNELS * sizeof(audio_chunk[0]);
	ioe_write(AM_AUDIO_PLAY, &play);
}

static void am_poll_input(void) {
	AM_INPUT_KEYBRD_T ev;

	do {
		ioe_read(AM_INPUT_KEYBRD, &ev);
		if (ev.keycode == AM_KEY_NONE) {
			break;
		}
		am_handle_key_event(&ev);
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

static void am_print_available_roms(void) {
	int i;

	printf("Available ROMs:\n");
	if (nroms <= 0) {
		printf("  (none)\n");
		return;
	}

	for (i = 0; i < nroms; ++i) {
		printf("  %s\n", roms[i].name);
	}
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

	return NULL;
}

static bool am_load_rom(const struct embedded_rom* rom) {
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

static bool am_load_save(const struct embedded_rom* rom) {
#if defined(ENABLE_VFS) && defined(ENABLE_VFS_FILE)
	char save_path[PATH_MAX];

	if (!rom) {
		return false;
	}

	snprintf(save_path, sizeof(save_path), "%s.sav", rom->name);
	if (!mCoreLoadSaveFile(core, save_path, false)) {
		printf("Failed to load save: %s\n", save_path);
		return false;
	}
	return true;
#else
	UNUSED(rom);
	return true;
#endif
}

int main(const char* args) {
	const struct embedded_rom* rom;
	bool has_rom_arg = args && *args;

	ioe_init();
	am_init_video();
	running = true;
	fps_window_start_us = 0;
	fps_window_frames = 0;
	fps_line_active = false;
	core = GBACoreCreate();
	assert(core);
	assert(core->init(core));
	mCoreInitConfig(core, "am");
	mStandardLoggerInit(&logger);
	mCoreConfigSetIntValue(&core->config, "logToStdout", 1);
	mCoreConfigSetIntValue(&core->config, "logToFile", 0);
	mCoreConfigSetIntValue(&core->config, "logLevel", mLOG_FATAL | mLOG_ERROR | mLOG_WARN | mLOG_GAME_ERROR);
	mCoreConfigSetIntValue(&core->config, "logLevel.gba.dma", 0);
	mCoreConfigSetIntValue(&core->config, "logLevel.gba.bios", 0);
	mCoreConfigSetIntValue(&core->config, "logLevel.gba.io", 0);
	mCoreConfigSetIntValue(&core->config, "logLevel.gba.memory", 0);
	mCoreConfigSetIntValue(&core->config, "logLevel.gba.sio", 0);
	mCoreConfigSetIntValue(&core->config, "logLevel.gba.hardware", 0);
	mStandardLoggerConfig(&logger, &core->config);
	mLogSetDefaultLogger(&logger.d);
	core->setAudioBufferSize(core, 2048);
	core->setVideoBuffer(core, framebuffer, FB_W);
	rom = am_select_rom(args);
	if (has_rom_arg && !rom) {
		printf("Unknown ROM: %s\n", args);
		am_print_available_roms();
		mLogSetDefaultLogger(NULL);
		mStandardLoggerDeinit(&logger);
		core->deinit(core);
		return 1;
	}

	if (!am_load_rom(rom)) {
		core->deinit(core);
		return 1;
	}

	if (!am_load_save(rom)) {
		core->deinit(core);
		return 1;
	}
	am_init_audio();
	core->reset(core);
	am_init_timing();

	while (running) {
		uint32_t keys;
		am_poll_input();
		keys = am_build_gba_keys();
		core->setKeys(core, keys);
		core->runFrame(core);
		am_flush_audio();
		am_flush_video();
		am_report_fps();
		am_throttle_frame();
	}

	mLogSetDefaultLogger(NULL);
	am_deinit_audio();
	mStandardLoggerDeinit(&logger);
	core->deinit(core);
	if (fps_line_active) {
		printf("\n");
	}
	return 0;
}
