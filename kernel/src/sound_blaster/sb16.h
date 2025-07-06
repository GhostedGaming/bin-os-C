#ifndef SB16_H
#define SB16_H

#pragma once

#include <stdint.h>
#include <stdbool.h>

#define BUF_SIZE 512

// I/O Ports
#define SB16_RESET         0x226
#define SB16_READ_STATUS   0x22E
#define SB16_READ_DATA     0x22A
#define SB16_WRITE_CMD     0x22C

// Commands
#define SB16_CMD_SPEAKER_ON     0xD1
#define SB16_CMD_SPEAKER_OFF    0xD3
#define SB16_CMD_SET_RATE       0x41
#define SB16_CMD_PLAY_DMA_8     0xC6

bool sb16_reset(void);
void sb16_speaker_on(void);
void sb16_speaker_off(void);
void sb16_set_sample_rate(uint16_t hz);
void sb16_dma_setup(uint8_t *buffer, uint16_t length);
void sb16_play(uint8_t *buffer, uint16_t length, uint16_t sample_rate);
void sb16_beep(uint32_t freq, uint32_t duration_ms);

#endif // SB16_H