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

#ifndef OLED_H_
#define OLED_H_

#include <stdlib.h>

#define NO_FONT 0
#define SMALL_FONT 1
#define LARGE_FONT 2

#define LARGE_FONT_WIDTH    14
#define LARGE_FONT_HEIGHT   15
#define LARGE_FONT_SPAN     (LARGE_FONT_HEIGHT/8)+1
#define SMALL_FONT_WIDTH    5

class OLED {
    public:
        OLED(uint8_t reset_pin, uint8_t i2c_address);

        void init();
        void clear();
        void set_cursor(uint8_t column, uint8_t row, uint8_t size_flag);
        void write(char *format, ...);

    private:
        uint8_t p_reset_pin, p_i2c_address;

        void send_command(uint8_t c);
        void send_data(uint8_t c);
        
        void set_start_column(uint8_t column);
        void set_start_page(uint8_t page);

        void fill_ram(uint8_t data);
        void fill_ram_page(uint8_t page, uint8_t data);
        void fill_ram_font_small(uint8_t number, uint8_t column, uint8_t row);
        void fill_ram_font(uint8_t number, uint8_t column, uint8_t row, uint8_t span, uint8_t width);

        uint8_t p_column, p_row;
        uint8_t p_column_increment_small_font, p_column_increment_large_font;
        uint8_t p_column_increment_flag;
        unsigned long dv[10]; // how to do it with const?

        void putc(uint8_t c);
        void puts(char* s);
        void xtoa(unsigned long x, const unsigned long *dp);
        void puth(unsigned n);
};

#endif
