/*
 * Copyright 2012 Andres Vahter
 * 
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * This driver is based on www.43oh.com 128x64 OLED Booster Pack driver:
 * http://www.43oh.com/2011/11/the-terminal-128x64-oled-booster-pack/
 *
 * Original driver is heaviliy modified by Andres Vahter in order to work
 * with Energia environment and with I2C protocol.
*/

#include <Energia.h>
#include <Wire.h>
#include "OLED.h"
#include "CourierNew_14x15.h"
#include "CourierNew_5x7.h"

OLED::OLED(uint8_t reset_pin, uint8_t i2c_address) {
    p_reset_pin = reset_pin;
    p_i2c_address = i2c_address;

    p_row = 0;
    p_column = 0;
    p_column_increment_small_font = 0;
    p_column_increment_large_font = 0;
    p_column_increment_flag = NO_FONT;

    //      4294967296      // 32 bit unsigned max
    dv[0] = 1000000000;     // +0
    dv[1] =  100000000;     // +1
    dv[2] =   10000000;     // +2
    dv[3] =    1000000;     // +3
    dv[4] =     100000;     // +4
    //           65535      // 16 bit unsigned max     
    dv[5] =      10000;     // +5
    dv[6] =       1000;     // +6
    dv[7] =        100;     // +7
    dv[8] =         10;     // +8
    dv[9] =          1;     // +9
        
}

void OLED::init() {
    pinMode(p_reset_pin, OUTPUT);
    digitalWrite(p_reset_pin, HIGH);
    delay(100);

    Wire.begin();

    digitalWrite(p_reset_pin, HIGH);
    // VDD (3.3V) goes high at start, lets just chill for a ms
    delay(1);
    digitalWrite(p_reset_pin, LOW);
    delay(10);
    digitalWrite(p_reset_pin, HIGH);

    send_command(0xae); // turn off oled panel 
    send_command(0x00); // set low column address 
    send_command(0x10); // set high column address 
    send_command(0x40); // set start line address 
    send_command(0x81); // set contrast control register 
    send_command(0xcf); 
    send_command(0xa1); // set segment re-map 95 to 0 
    send_command(0xa6); // set normal display 
    send_command(0xa8); // set multiplex ratio(1 to 64) 
    send_command(0x3f); // 1/64 duty 
    send_command(0xd3); // set display offset 
    send_command(0x00); // not offset 
    send_command(0xd5); // set display clock divide ratio/oscillator frequency 
    send_command(0x80); // set divide ratio 
    send_command(0xd9); // set pre-charge period 
    send_command(0xf1); 
    send_command(0xda); // set com pins hardware configuration 
    send_command(0x12); 
    send_command(0xdb); // set vcomh 
    send_command(0x40); 
    send_command(0x8d); // set Charge Pump enable/disable 
    send_command(0x14); // set(0x10) disable 
    send_command(0xaf); // turn on oled panel
    
    clear();            // Clear Screen
}

void OLED::clear() {
    fill_ram(0x00);
}

void OLED::set_cursor(uint8_t column, uint8_t row, uint8_t size_flag) {
    p_row = row;
    p_column_increment_flag = size_flag;
  
    if (p_column_increment_flag == SMALL_FONT)
        p_column_increment_small_font = column;
  
    if (p_column_increment_flag == LARGE_FONT)
        p_column_increment_large_font = column;
}

void OLED::write(char *format, ...) {
    char c;
    int i, val;
    int8_t padding_len;
    long n;
    
    va_list a;
    va_start(a, format);
    while (c = *format++) {
        if (c == '%') {
            c = *format++;
            // '1' is 49 '2' is 50 .. '9' is 57
            // '1' - 48 = 0x01 which is real number, not ASCII number
            padding_len = c-48;
            // we found padding so now check format string type
            if ((uint8_t)padding_len >= 1 && (uint8_t)padding_len <= 9) {
                c = *format++;
            }

            switch (c) {
                case 's':                       // String
                    puts(va_arg(a, char*));
                    break;
                case 'c':                       // Char
                    putc(va_arg(a, char));
                    break;
                case 'i':                       // 16 bit Integer
                case 'd':                       // 16 bit Integer
                case 'u':                       // 16 bit Unsigned
                    i = va_arg(a, int);
                    if ((uint8_t)padding_len >= 1 && (uint8_t)padding_len <= 9) {
                        val = i;
                        while (val != 0) {
                            val = val / 10;
                            padding_len--;
                        }

                        if (padding_len > 0) {
                            if (i <= 0) {
                                padding_len--; // - sign already pads 1 place and 0 is special
                            }

                            for (int i = 0; i < (uint8_t)padding_len; ++i) {
                                putc(' ');
                            }
                        }
                    }

                    if (c == 'i' && i < 0) {
                        i = -i;
                        putc('-');
                    }

                    xtoa((unsigned)i, dv + 5);
                    break;
                case 'l':                       // 32 bit Long
                case 'n':                       // 32 bit uNsigned loNg
                    n = va_arg(a, long);
                    if (c == 'l' &&  n < 0) n = -n, putc('-');
                    xtoa((unsigned long)n, dv);
                    break;
                case 'x':                       // 16 bit heXadecimal
                    i = va_arg(a, int);
                    puth(i >> 12);
                    puth(i >> 8);
                    puth(i >> 4);
                    puth(i);
                    break;
                case 0: return;
                default: goto bad_fmt;
            }
        }
        else {
            bad_fmt:    putc(c);            
        }
    }
    va_end(a);
}

void OLED::putc(uint8_t c) {
    if (p_column_increment_flag == SMALL_FONT) {
        fill_ram_font_small(c-32, p_column_increment_small_font, p_row);
        p_column_increment_small_font = p_column_increment_small_font + SMALL_FONT_WIDTH;
    }

    if (p_column_increment_flag == LARGE_FONT) {
        fill_ram_font(c, p_column_increment_large_font, p_row, LARGE_FONT_SPAN, LARGE_FONT_WIDTH);
        p_column_increment_large_font = p_column_increment_large_font + LARGE_FONT_WIDTH;
    } 
}
void OLED::puts(char* s) {
    while(*s) {
        if (p_column_increment_flag == SMALL_FONT) {
            fill_ram_font_small(*s - 32, p_column_increment_small_font, p_row);
            p_column_increment_small_font = p_column_increment_small_font + SMALL_FONT_WIDTH;
        }

        if (p_column_increment_flag == LARGE_FONT) {
            fill_ram_font(*s, p_column_increment_large_font, p_row, LARGE_FONT_SPAN, LARGE_FONT_WIDTH);
            p_column_increment_large_font = p_column_increment_large_font + LARGE_FONT_WIDTH;
        }
        *s++;
    }
}

void OLED::xtoa(unsigned long x, const unsigned long *dp) {
    char c;
    unsigned long d;
    if (x) {
        while(x < *dp) ++dp;
        do {
            d = *dp++;
            c = '0';
            while(x >= d) ++c, x -= d;
            putc(c);
        } while(!(d & 1));
    }
    else {
        putc('0');
    }
}

void OLED::puth(unsigned n) {
    static const char hex[16] = { '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
    putc(hex[n & 15]);
}

void OLED::send_command(uint8_t c) {
    Wire.beginTransmission(p_i2c_address);
    Wire.write(0x00);
    Wire.write(c);
    Wire.endTransmission();
}

void OLED::send_data(uint8_t c) {
    Wire.beginTransmission(p_i2c_address);
    Wire.write(0x40);
    Wire.write(c);
    Wire.endTransmission();
}

void OLED::set_start_column(uint8_t column) {
    // Set Lower Column Start Address for Page Addressing Mode. Default => 0x00
    send_command(0x00 + column % 16);
    
    // Set Higher Column Start Address for Page Addressing Mode. Default => 0x10
    send_command(0x10 + column / 16);
}

void OLED::set_start_page(uint8_t page) {
    // Set Page Start Address for Page Addressing Mode. Default => 0xB0 (0x00)
    send_command(0xB0 | page);
}

void OLED::fill_ram(uint8_t data) {
    uint8_t i, j;

    for (i=0; i<8; i++) {
        set_start_page(i);
        set_start_column(0x00);

        for (j=0; j<128; j++) {
            send_data(data);
        }
    }
}

void OLED::fill_ram_page(uint8_t page, uint8_t data) {
    uint8_t j;

    set_start_page(page);
    set_start_column(0x00);

    for (j=0; j<128; j++) {
        send_data(data);
    }
}

void OLED::fill_ram_font_small(uint8_t number, uint8_t column, uint8_t row) {
    uint8_t j = 0;
    set_start_page(row);
    set_start_column(column);
    
    for (j=0; j<SMALL_FONT_WIDTH; j++) {
        send_data(font_table_small[j + (number * SMALL_FONT_WIDTH)]);
    }
}

void OLED::fill_ram_font(uint8_t number, uint8_t column, uint8_t row, uint8_t span, uint8_t width) {
    int i, j, test;
    int adder = 0;
    int jump = 0;

    jump = (number - 32);
    
    for (i=row; i>=row-span+1; i--) {
        set_start_page(i);
        set_start_column(column);
    
        for (j=0; j<width; j++) {
            test = j + (adder * width) + (jump * 28);
            send_data(font_table_large[test]);
        }
        adder++;
    }
}
