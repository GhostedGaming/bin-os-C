#include "sb16.h"
#include "../timing/timer.h"
#include "../io.h"
#include "../serial/serial.h"

__attribute__((section(".dma"), aligned(16)))
static uint8_t audio_buffer[BUF_SIZE];

// === Reset DSP ===
bool sb16_reset(void) {
    outb(SB16_RESET, 1);
    timer_wait_micros(3);
    outb(SB16_RESET, 0);

    for (int i = 0; i < 1000; i++) {
        if (inb(SB16_READ_STATUS) & 0x80) {
            uint8_t resp = inb(SB16_READ_DATA);
            if (resp == 0xAA) {
                write_serial("SB16: Reset OK\r\n");
                return true;
            } else {
                write_serial("SB16: Unexpected DSP response\r\n");
                return false;
            }
        }
        timer_wait_micros(10);
    }

    write_serial("SB16: Reset timeout\r\n");
    return false;
}

// === Speaker Control ===
void sb16_speaker_on(void) {
    outb(SB16_WRITE_CMD, SB16_CMD_SPEAKER_ON);
}

void sb16_speaker_off(void) {
    outb(SB16_WRITE_CMD, SB16_CMD_SPEAKER_OFF);
}

// === Set Sample Rate ===
void sb16_set_sample_rate(uint16_t hz) {
    outb(SB16_WRITE_CMD, SB16_CMD_SET_RATE);
    outb(SB16_WRITE_CMD, (hz >> 8) & 0xFF);
    outb(SB16_WRITE_CMD, hz & 0xFF);
}

// === Setup DMA (8-bit, channel 1) ===
void sb16_dma_setup(uint8_t *buffer, uint16_t length) {
    uint32_t addr = (uint32_t)buffer;

    if (addr >= 0x100000) {
        write_serial("SB16: DMA buffer not in <1MB!\r\n");
        return;
    }

    uint16_t offset = addr & 0xFFFF;
    uint8_t page = (addr >> 16) & 0xFF;
    uint16_t count = length - 1;

    outb(0x0A, 0x05);        // Disable channel 1
    outb(0x0C, 0xFF);        // Clear pointer
    outb(0x0B, 0x49);        // Mode: Single, read, increment, ch 1

    outb(0x02, offset & 0xFF);
    outb(0x02, offset >> 8);
    outb(0x83, page);

    outb(0x03, count & 0xFF);
    outb(0x03, count >> 8);

    outb(0x0A, 0x01);        // Enable channel 1
}

// === Play buffer via DSP ===
void sb16_play(uint8_t *buffer, uint16_t length, uint16_t sample_rate) {
    if (!sb16_reset()) return;

    sb16_speaker_on();
    sb16_set_sample_rate(sample_rate);
    sb16_dma_setup(buffer, length);

    outb(SB16_WRITE_CMD, SB16_CMD_PLAY_DMA_8);
    outb(SB16_WRITE_CMD, (length - 1) & 0xFF);
    outb(SB16_WRITE_CMD, (length - 1) >> 8);

    write_serial("SB16: Playing buffer\r\n");
}

// === Generate and play square wave (frequency + duration) ===
void sb16_beep(uint32_t freq, uint32_t duration_ms) {
    uint32_t sample_rate = 22050;
    uint32_t samples_per_cycle = sample_rate / freq;
    if (samples_per_cycle < 2) samples_per_cycle = 2;

    for (uint16_t i = 0; i < BUF_SIZE; i++) {
        audio_buffer[i] = (i % samples_per_cycle < samples_per_cycle / 2) ? 0xFF : 0x00;
    }

    sb16_play(audio_buffer, BUF_SIZE, sample_rate);
    timer_wait_ms(duration_ms);
    sb16_speaker_off();
}