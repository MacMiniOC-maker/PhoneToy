// I2S audio: ring tone playback and mic -> earpiece loopback.
// Audio only ever flows through RAM buffers; nothing is stored.
#pragma once

typedef enum { AUDIO_IDLE, AUDIO_RING, AUDIO_LOOPBACK } audio_mode_t;

void audio_init(void);
void audio_set_mode(audio_mode_t mode);
// Peak mic level (0..32767) since the last call. Only updated in loopback.
int audio_take_mic_peak(void);
