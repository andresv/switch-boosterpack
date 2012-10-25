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

#include <Energia.h>
#include <stdlib.h>
#include "OLED.h"

#define PLUS_BUTTON 4
#define MINUS_BUTTON 5
#define SELECT_BUTTON 6

OLED oled(13, 0x3C);

int8_t current_temp_1;
int8_t set_temp_1;
int8_t current_temp_2;
int8_t set_temp_2;

uint8_t relay_states = 0; //0b000x xxx0

enum {
    STATE_DISPLAY,
    STATE_PLUS_BUTTON_PRESSED,
    STATE_MINUS_BUTTON_PRESSED,
    STATE_SELECT_BUTTON_PRESSED,
};

enum {
    MODE_DISPLAY,
    MODE_SET_1,
    MODE_SET_2,
};

uint8_t state = STATE_DISPLAY;
uint8_t mode = MODE_DISPLAY;

void read_temp_sensors() {
    current_temp_1 = 25;
    oled.set_cursor(10, 5, LARGE_FONT);
    oled.write("%3i %3i", current_temp_1, set_temp_1);
    
    current_temp_2 = 40;
    oled.set_cursor(10, 2, LARGE_FONT);
    oled.write("%3i %3i", current_temp_2, set_temp_2);
}

void display_relay_state(uint8_t relay_nr, bool on) {
    if (on) {
        relay_states |= (1<<relay_nr);
        oled.write("(O)");
    }
    else {
        relay_states &= ~(1<<relay_nr);
        oled.write("( )");
    }
}

void switch_relay(uint8_t relay_nr, bool on) {
    switch (relay_nr) {
        case 1:
            oled.set_cursor(114, 7, SMALL_FONT);
            display_relay_state(relay_nr, on);
            break;
        case 2:
            oled.set_cursor(114, 5, SMALL_FONT);
            display_relay_state(relay_nr, on);
            break;
        case 3:
            oled.set_cursor(114, 3, SMALL_FONT);
            display_relay_state(relay_nr, on);
            break;
        case 4:
            oled.set_cursor(114, 1, SMALL_FONT);
            display_relay_state(relay_nr, on);
            break;
    }
}

void setup() {
    oled.init();
    oled.clear();
    
    oled.set_cursor(30, 7, SMALL_FONT);
    oled.write("cur       set");

    oled.set_cursor(10, 5, LARGE_FONT);
    oled.write("%3i %3i", current_temp_1, set_temp_1);

    oled.set_cursor(10, 2, LARGE_FONT);
    oled.write("%3i %3i", current_temp_2, set_temp_2);

    switch_relay(1, HIGH);
    switch_relay(2, LOW);
    switch_relay(3, HIGH);
    switch_relay(4, HIGH);
    
    pinMode(PLUS_BUTTON, INPUT_PULLUP);
    pinMode(MINUS_BUTTON, INPUT_PULLUP);
    pinMode(SELECT_BUTTON, INPUT_PULLUP);
}

void loop() {
    read_temp_sensors();

    if (digitalRead(PLUS_BUTTON) == LOW) {
        state = STATE_PLUS_BUTTON_PRESSED;
        delay(10);
    }
    else if (digitalRead(MINUS_BUTTON) == LOW) {
        state = STATE_MINUS_BUTTON_PRESSED;
        delay(10);
    }
    else if (digitalRead(SELECT_BUTTON) == LOW) {
        state = STATE_SELECT_BUTTON_PRESSED;
        delay(10);
    }

    switch(state) {
        case STATE_PLUS_BUTTON_PRESSED:
            if (mode == MODE_SET_1) {
                set_temp_1++;
            }
            else if (mode == MODE_SET_2) {
                set_temp_2++;
            }
            state = STATE_DISPLAY;
            break;

        case STATE_MINUS_BUTTON_PRESSED:
            if (mode == MODE_SET_1) {
                set_temp_1--;
            }
            else if (mode == MODE_SET_2) {
                set_temp_2--;
            }
            state = STATE_DISPLAY;
            break;

        case STATE_SELECT_BUTTON_PRESSED:
            if (mode == MODE_DISPLAY) {
                mode = MODE_SET_1;
            }
            else if (mode == MODE_SET_1) {
                mode = MODE_SET_2;
            }
            else {
                mode = MODE_DISPLAY;
                delay(500); // wait a little, otherwise we might get back to MODE_SET_1 too soon
            }

            state = STATE_DISPLAY;
            break;

        case STATE_DISPLAY:
            if (mode == MODE_SET_1) {
                oled.set_cursor(10, 5, LARGE_FONT);
                oled.write("%3i    ", current_temp_1);
                oled.set_cursor(10, 5, LARGE_FONT);
                oled.write("%3i %3i", current_temp_1, set_temp_1);
            }
            else if (mode == MODE_SET_2) {
                oled.set_cursor(10, 2, LARGE_FONT);
                oled.write("%3i    ", current_temp_2);
                oled.set_cursor(10, 2, LARGE_FONT);
                oled.write("%3i %3i", current_temp_2, set_temp_2);
            }

            break;
    }
}
