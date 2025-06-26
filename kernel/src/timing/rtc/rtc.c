#include "../../io.h"
#include "rtc.h"
#include "../../serial/serial.h"

extern void disable_ints();
extern void enable_ints();

volatile int rtc_interrupt_received = 0;

unsigned char read_rtc_register(unsigned char reg) {
    outb(RTC_INDEX, reg);
    return inb(RTC_DATA);
}

void write_rtc_register(unsigned char reg, unsigned char value) {
    outb(RTC_INDEX, reg);
    outb(RTC_DATA, value);
}

int rtc_is_updating() {
    outb(RTC_INDEX, RTC_REG_A);
    return (inb(RTC_DATA) & 0x80);
}

void wait_for_rtc_update() {
    while (rtc_is_updating());
}

unsigned char bcd_to_binary(unsigned char bcd) {
    return ((bcd & 0xF0) >> 1) + ((bcd & 0xF0) >> 3) + (bcd & 0x0F);
}

void read_rtc_time(unsigned char *second, unsigned char *minute, unsigned char *hour, 
                   unsigned char *day, unsigned char *month, unsigned char *year) {
    write_serial("read_rtc_time");
    unsigned char century;
    unsigned char last_second, last_minute, last_hour, last_day, last_month, last_year, last_century;
    unsigned char registerB;

    wait_for_rtc_update();

    do {
        last_second = read_rtc_register(RTC_SECONDS);
        last_minute = read_rtc_register(RTC_MINUTES);
        last_hour = read_rtc_register(RTC_HOURS);
        last_day = read_rtc_register(RTC_DAY);
        last_month = read_rtc_register(RTC_MONTH);
        last_year = read_rtc_register(RTC_YEAR);
        last_century = read_rtc_register(RTC_CENTURY);

        wait_for_rtc_update();

        *second = read_rtc_register(RTC_SECONDS);
        *minute = read_rtc_register(RTC_MINUTES);
        *hour = read_rtc_register(RTC_HOURS);
        *day = read_rtc_register(RTC_DAY);
        *month = read_rtc_register(RTC_MONTH);
        *year = read_rtc_register(RTC_YEAR);
        century = read_rtc_register(RTC_CENTURY);
    } while ((last_second != *second) || (last_minute != *minute) || (last_hour != *hour) ||
             (last_day != *day) || (last_month != *month) || (last_year != *year) ||
             (last_century != century));

    registerB = read_rtc_register(RTC_REG_B);

    if (!(registerB & 0x04)) {
        *second = bcd_to_binary(*second);
        *minute = bcd_to_binary(*minute);
        *hour = bcd_to_binary(*hour & 0x7F) | (*hour & 0x80);
        *day = bcd_to_binary(*day);
        *month = bcd_to_binary(*month);
        *year = bcd_to_binary(*year);
        century = bcd_to_binary(century);
    }

    if (!(registerB & 0x02) && (*hour & 0x80)) {
        *hour = ((*hour & 0x7F) + 12) % 24;
    }

    if (century == 0) century = 20;
    *year += century * 100;
}

void rtc_interrupt_handler() {
    read_rtc_register(RTC_REG_C);
    rtc_interrupt_received = 1;
}

void enable_irq8() {
    disable_ints();
    outb(RTC_INDEX, RTC_REG_B);
    char prev = inb(RTC_DATA);
    outb(RTC_INDEX, RTC_REG_B);
    outb(RTC_DATA, prev | 0x40);
    enable_ints();
}

void disable_irq8() {
    disable_ints();
    outb(RTC_INDEX, RTC_REG_B);
    char prev = inb(RTC_DATA);
    outb(RTC_INDEX, RTC_REG_B);
    outb(RTC_DATA, prev & 0xBF);
    enable_ints();
}

void set_rtc_frequency(unsigned char rate) {
    disable_ints();
    outb(RTC_INDEX, RTC_REG_A);
    char prev = inb(RTC_DATA);
    outb(RTC_INDEX, RTC_REG_A);
    outb(RTC_DATA, (prev & 0xF0) | rate);
    enable_ints();
}

void init_rtc() {
    write_serial("init_rtc: Disabling Ints");
    disable_ints();
    write_serial("init_rtc: Disabling Ints completed");
    outb(RTC_INDEX, RTC_REG_A);
    outb(RTC_DATA, 0x26);
    enable_ints();
    enable_irq8();
}

void wait_for_rtc_interrupt() {
    rtc_interrupt_received = 0;
    while (!rtc_interrupt_received);
}