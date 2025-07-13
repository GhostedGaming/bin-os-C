#ifndef RTC_H
#define RTC_H

#define RTC_SECONDS     0x00
#define RTC_MINUTES     0x02
#define RTC_HOURS       0x04
#define RTC_DAY         0x07
#define RTC_MONTH       0x08
#define RTC_YEAR        0x09
#define RTC_CENTURY     0x32
#define RTC_REG_A       0x8A
#define RTC_REG_B       0x8B
#define RTC_REG_C       0x8C
#define RTC_INDEX       0x70
#define RTC_DATA        0x71

extern volatile int rtc_interrupt_received;

unsigned char read_rtc_register(unsigned char reg);
void write_rtc_register(unsigned char reg, unsigned char value);
int rtc_is_updating(void);
void wait_for_rtc_update(void);
unsigned char bcd_to_binary(unsigned char bcd);
void read_rtc_time(unsigned char *second, unsigned char *minute, unsigned char *hour, 
                   unsigned char *day, unsigned char *month, unsigned char *year);
void rtc_interrupt_handler(void);
void enable_irq8(void);
void disable_irq8(void);
void set_rtc_frequency(unsigned char rate);
void init_rtc(void);
void wait_for_rtc_interrupt(void);
void display_current_time();
void display_time_components();

#endif