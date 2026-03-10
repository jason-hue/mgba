#include <mgba/core/core.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/audio.h>
#include <mgba/internal/gb/audio.h>
#include <mgba/internal/gba/sio/gbp.h>
#include <mgba/core/timing.h>
#include <mgba-util/audio-buffer.h>

static void _dummyCallback(struct mTiming* timing, void* context, uint32_t cyclesLate) {
    UNUSED(timing);
    UNUSED(context);
    UNUSED(cyclesLate);
}

// Audio Stubs - Matching include/mgba/internal/gb/audio.h
void GBAudioInit(struct GBAudio* audio, size_t samples, uint8_t* nr52, enum GBAudioStyle style) {
    (void)nr52; (void)style;
    audio->frameEvent.callback = _dummyCallback;
    audio->frameEvent.context = audio;
    audio->sampleEvent.callback = _dummyCallback;
    audio->sampleEvent.context = audio;
    
    // Initialize the buffer to prevent division by zero in mAudioBufferWrite
    // GBA uses 2 channels
    mAudioBufferInit(&audio->buffer, samples, 2);
}

void GBAudioReset(struct GBAudio* audio) { (void)audio; }
void GBAudioRun(struct GBAudio* audio, int32_t timestamp, int channels) { (void)audio; (void)timestamp; (void)channels; }
void GBAudioDeinit(struct GBAudio* audio) { 
    mAudioBufferDeinit(&audio->buffer);
}

void GBAudioWriteNR10(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR11(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR12(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR13(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR14(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR21(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR22(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR23(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR24(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR30(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR31(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR33(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR34(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR41(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR42(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR43(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR44(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR50(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR51(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }
void GBAudioWriteNR52(struct GBAudio* audio, uint8_t v) { (void)audio; (void)v; }

void GBAudioSamplePSG(struct GBAudio* audio, int16_t* left, int16_t* right) { (void)audio; (void)left; (void)right; }
void GBAudioPSGSerialize(const struct GBAudio* audio, struct GBSerializedPSGState* state, uint32_t* flagsOut) { (void)audio; (void)state; (void)flagsOut; }
void GBAudioPSGDeserialize(struct GBAudio* audio, const struct GBSerializedPSGState* state, const uint32_t* flagsIn) { (void)audio; (void)state; (void)flagsIn; }

// Video Logger Stubs
void mVideoLogCoreFind(void) {}
void mVideoLogContextInitialState(void) {}
void mVideoLogContextCreate(void) {}
void mVideoLogContextLoad(void) {}
void mVideoLogContextDestroy(void) {}
void mVideoLogContextRewind(void) {}
void mVideoLoggerAddChannel(void) {}
void mVideoLoggerAttachChannel(void) {}
void mVideoLoggerRendererCreate(void) {}
void mVideoLoggerRendererRun(void) {}
void mVideoLoggerRendererInit(void) {}
void mVideoLoggerRendererDeinit(void) {}
void mVideoLoggerRendererWritePalette(void) {}
void mVideoLoggerRendererWriteVRAM(void) {}
void mVideoLoggerRendererWriteOAM(void) {}
void mVideoLoggerRendererWriteVideoRegister(void) {}
void mVideoLoggerRendererFlush(void) {}
void mVideoLoggerRendererFinishFrame(void) {}
void mVideoLoggerRendererDrawScanline(void) {}
void mVideoLoggerRendererReset(void) {}

// Patching Stubs
void loadPatchIPS(void) {}
void loadPatchUPS(void) {}
