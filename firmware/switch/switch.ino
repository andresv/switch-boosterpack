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
#include "MspFlash.h"
#include <stdlib.h>
#include "OLED.h"

#define TRUE 1
#define FALSE 0
#define ON 1
#define OFF 0

//https://github.com/energia/Energia/blob/master/hardware/msp430/libraries/MspFlash/examples/flash_readwrite/flash_readwrite.ino
#define flash SEGMENT_D

#define PLUS_BUTTON 4
#define MINUS_BUTTON 5
#define SELECT_BUTTON 6
#define LOAD1 7
#define LOAD2 8
#define LOAD3 9
#define LOAD4 10

#define NUMTEMPS 103 // NTC table
#define ROLLING_AVERAGE_SIZE 10

#define UPPER_HYSTERESIS 5
#define LOWER_HYSTERESIS 5
#define HIGH_TEMP_ON_TIME 2 * 60000 // 1 minute
#define HIGH_TEMP 60
#define NORMAL_TEMP 20

#define DEFAULT_2_OFF_TEMP -50
#define DEFAULT_2_ON_TEMP 20

OLED oled(13, 0x3C);

int8_t current_temp_1;
int8_t set_temp_1;
int8_t preset_temperatures_1[3] = {NORMAL_TEMP, HIGH_TEMP, 80};
int8_t current_temp_2;
int8_t set_temp_2;

uint16_t temp_1_rolling_buf[ROLLING_AVERAGE_SIZE];
uint16_t temp_2_rolling_buf[ROLLING_AVERAGE_SIZE];

bool hysteresis_1_raising = TRUE;
bool hysteresis_2_raising = TRUE;
uint32_t high_temp_mode_start_time = 0;

uint8_t relay_states = 0; //0b000x xxx0

enum {
    STATE_DISPLAY,
    STATE_PLUS_MINUS_PRESSED,
    STATE_PLUS_BUTTON_PRESSED,
    STATE_MINUS_BUTTON_PRESSED,
    STATE_SELECT_BUTTON_PRESSED,
};

enum {
    MODE_DISPLAY,
};

uint8_t state = STATE_DISPLAY;
uint8_t mode = MODE_DISPLAY;
bool show_dot = 1;

void select_pressed();

// https://raw.github.com/reprap/firmware/master/createTemperatureLookup.py
// python createTemperatureLookup.py --r0=4700 --t0=25 --r1=0 --r2=1000 --beta=3950 --max-adc=1023
//      num steps were manualy set to 100 and adcref to 3.6 V
// http://hydraraptor.blogspot.com/2007/10/measuring-temperature-easy-way.html
// NTC 4.7k EPCOS B57045-K472-K

const short temptable[NUMTEMPS][2] = {
   {1, 554},
   {11, 277},
   {21, 231},
   {31, 206},
   {41, 190},
   {51, 178},
   {61, 168},
   {71, 160},
   {81, 154},
   {91, 148},
   {101, 143},
   {111, 138},
   {121, 134},
   {131, 130},
   {141, 127},
   {151, 123},
   {161, 120},
   {171, 118},
   {181, 115},
   {191, 113},
   {201, 110},
   {211, 108},
   {221, 106},
   {231, 104},
   {241, 102},
   {251, 100},
   {261, 98},
   {271, 96},
   {281, 95},
   {291, 93},
   {301, 91},
   {311, 90},
   {321, 88},
   {331, 87},
   {341, 85},
   {351, 84},
   {361, 82},
   {371, 81},
   {381, 80},
   {391, 78},
   {401, 77},
   {411, 76},
   {421, 75},
   {431, 73},
   {441, 72},
   {451, 71},
   {461, 70},
   {471, 69},
   {481, 67},
   {491, 66},
   {501, 65},
   {511, 64},
   {521, 63},
   {531, 62},
   {541, 61},
   {551, 60},
   {561, 58},
   {571, 57},
   {581, 56},
   {591, 55},
   {601, 54},
   {611, 53},
   {621, 52},
   {631, 51},
   {641, 50},
   {651, 49},
   {661, 47},
   {671, 46},
   {681, 45},
   {691, 44},
   {701, 43},
   {711, 42},
   {721, 41},
   {731, 39},
   {741, 38},
   {751, 37},
   {761, 36},
   {771, 35},
   {781, 33},
   {791, 32},
   {801, 31},
   {811, 29},
   {821, 28},
   {831, 26},
   {841, 25},
   {851, 23},
   {861, 22},
   {871, 20},
   {881, 19},
   {891, 17},
   {901, 15},
   {911, 13},
   {921, 11},
   {931, 8},
   {941, 6},
   {951, 3},
   {961, 0},
   {971, -2},
   {981, -6},
   {991, -11},
   {1001, -17},
   {1011, -27},
   {1021, -47}
};

int convert_raw_to_celsius(int rawtemp) {
    int current_celsius = 0;
    int i = 0;

    for (i=1; i < NUMTEMPS; i++) {
        if (temptable[i][0] > rawtemp) {
            int realtemp  = temptable[i-1][1] + (rawtemp - temptable[i-1][0]) * (temptable[i][1] - temptable[i-1][1]) / (temptable[i][0] - temptable[i-1][0]);
            if (realtemp > 255) {
                realtemp = 255;
            }

            current_celsius = realtemp;
            break;
        }
    }

    // Overflow: We just clamp to 0 degrees celsius
    if (i == NUMTEMPS)  {
        current_celsius = 1000;
    }

    return current_celsius;
}

inline uint16_t calc_average(uint16_t* buf, bool filled) {
    if (filled) {
        uint8_t i;
        uint16_t sum = 0;
        for (i=0; i<ROLLING_AVERAGE_SIZE; i++) {
            sum += buf[i];
        }
        return sum/ROLLING_AVERAGE_SIZE;
    }
    else {
        return buf[0];
    }
}

uint16_t temp_1_rolling_average(uint16_t rawtemp) {
    static uint8_t index = 0;
    static bool filled = FALSE;

    temp_1_rolling_buf[index] = rawtemp;
    index++;
    if (index == ROLLING_AVERAGE_SIZE) {
        index = 0;
        filled = TRUE;
    }

    return calc_average(temp_1_rolling_buf, filled);
}

uint16_t temp_2_rolling_average(uint16_t rawtemp) {
    static uint8_t index = 0;
    static bool filled = FALSE;

    temp_2_rolling_buf[index] = rawtemp;
    index++;
    if (index == ROLLING_AVERAGE_SIZE) {
        index = 0;
        filled = TRUE;
    }

    return calc_average(temp_2_rolling_buf, filled);
}

int read_temp_1() {
    uint16_t rawtemp = analogRead(A1);
    return convert_raw_to_celsius(temp_1_rolling_average(rawtemp));
}

int read_temp_2() {
    uint16_t rawtemp = analogRead(A0);
    return convert_raw_to_celsius(temp_2_rolling_average(rawtemp));
}

void read_temp_sensors() {
    current_temp_1 = read_temp_1();
    oled.set_cursor(10, 5, LARGE_FONT);
    oled.write("%3i %3i", current_temp_1, set_temp_1);
    
    current_temp_2 = read_temp_2();
    oled.set_cursor(10, 2, LARGE_FONT);
    if (set_temp_2 == DEFAULT_2_OFF_TEMP) {
        oled.write("%3i OFF", current_temp_2);
    }
    else {
        oled.write("%3i %3i", current_temp_2, set_temp_2);
    }
}

void display_relay_state(uint8_t relay_nr, bool on) {
    if (on) {
        relay_states |= (1<<relay_nr);
        oled.write("o");
    }
    else {
        relay_states &= ~(1<<relay_nr);
        oled.write(".");
    }
}

void switch_relay(uint8_t relay_nr, bool on) {
    // Notice that LOAD outputs are inverted
    // LOW is ON
    // HIGH is OFF
    // Thats why there is such a 'nice' bit twiddling
    switch (relay_nr) {
        case 1:
            oled.set_cursor(120, 7, SMALL_FONT);
            display_relay_state(relay_nr, on);
            digitalWrite(LOAD1, ((relay_states ^ (1 << 1)) >> 1) & 0x01);
            break;
        case 2:
            oled.set_cursor(120, 5, SMALL_FONT);
            display_relay_state(relay_nr, on);
            digitalWrite(LOAD2, ((relay_states ^ (1 << 2)) >> 2) & 0x01);
            break;
        case 3:
            oled.set_cursor(120, 3, SMALL_FONT);
            display_relay_state(relay_nr, on);
            digitalWrite(LOAD3, ((relay_states ^ (1 << 3)) >> 3) & 0x01);
            break;
        case 4:
            oled.set_cursor(120, 1, SMALL_FONT);
            display_relay_state(relay_nr, on);
            digitalWrite(LOAD4, ((relay_states ^ (1 << 4)) >> 4) & 0x01);
            break;
    }
}

void save_set_values() {
    int8_t buf[2] = {set_temp_1, set_temp_2};
    Flash.erase(flash);
    Flash.write(flash, (unsigned char*)buf, 2);
}

void restore_set_values() {
    Flash.read(flash, (unsigned char*)&set_temp_1, 1);
    Flash.read(flash+1, (unsigned char*)&set_temp_2, 1);
}

void setup() {
    restore_set_values();

    oled.init();
    oled.clear();
    
    oled.set_cursor(15, 7, SMALL_FONT);
    oled.write("current       set");

    oled.set_cursor(10, 5, LARGE_FONT);
    oled.write("%3i %3i", current_temp_1, set_temp_1);

    oled.set_cursor(10, 2, LARGE_FONT);
    if (set_temp_2 == DEFAULT_2_OFF_TEMP) {
        oled.write("%3i OFF", current_temp_2);
    }
    else {
        oled.write("%3i %3i", current_temp_2, set_temp_2);
    }

    pinMode(LOAD1, OUTPUT);
    pinMode(LOAD2, OUTPUT);
    pinMode(LOAD3, OUTPUT);
    pinMode(LOAD4, OUTPUT);
    switch_relay(1, OFF);
    switch_relay(2, OFF);
    switch_relay(3, OFF);
    switch_relay(4, OFF);
    
    pinMode(PLUS_BUTTON, INPUT_PULLUP);
    pinMode(MINUS_BUTTON, INPUT_PULLUP);
    pinMode(SELECT_BUTTON, INPUT_PULLUP);
    attachInterrupt(PLUS_BUTTON, &plus_pressed, FALLING);
    attachInterrupt(MINUS_BUTTON, &minus_pressed, FALLING);
    attachInterrupt(SELECT_BUTTON, &select_pressed, FALLING);
    
    pinMode(A0, INPUT);
    pinMode(A1, INPUT);
}

void plus_pressed() {
    state = STATE_PLUS_BUTTON_PRESSED;
}

void minus_pressed() {
    state = STATE_MINUS_BUTTON_PRESSED;
}

void select_pressed() {
    state = STATE_SELECT_BUTTON_PRESSED;
}

void loop() {
    static uint8_t temp_1_preset_index = 0;
    static bool default_off = TRUE;
    read_temp_sensors();

    // we have interrupts, but we also want to increment value
    // by just holding down a button
    if (digitalRead(PLUS_BUTTON) == LOW && digitalRead(MINUS_BUTTON) == LOW) {
        state = STATE_PLUS_MINUS_PRESSED;
    }
    else if (digitalRead(PLUS_BUTTON) == LOW) {
        state = STATE_PLUS_BUTTON_PRESSED;
    }
    else if (digitalRead(MINUS_BUTTON) == LOW) {
        state = STATE_MINUS_BUTTON_PRESSED;
    }
    else if (digitalRead(SELECT_BUTTON) == LOW) {
        state = STATE_SELECT_BUTTON_PRESSED;
    }

    switch(state) {
        case STATE_PLUS_MINUS_PRESSED:
            if (default_off) {
                default_off = FALSE;
                set_temp_2 = DEFAULT_2_ON_TEMP;
            }
            else {
                default_off = TRUE;
                set_temp_2 = DEFAULT_2_OFF_TEMP;
            }

            save_set_values();
            state = STATE_DISPLAY;
            delay(100);
            break;

        case STATE_PLUS_BUTTON_PRESSED:
            set_temp_2++;
            save_set_values();
            state = STATE_DISPLAY;
            delay(10); // debounce
            break;

        case STATE_MINUS_BUTTON_PRESSED:
            set_temp_2--;
            save_set_values();
            state = STATE_DISPLAY;
            delay(10); // debounce
            break;

        case STATE_SELECT_BUTTON_PRESSED:
            set_temp_1 = preset_temperatures_1[temp_1_preset_index];
            if (set_temp_1 == HIGH_TEMP) {
                high_temp_mode_start_time = millis();
            }

            temp_1_preset_index++;
            if (temp_1_preset_index >= sizeof(preset_temperatures_1)/sizeof(int8_t)) {
                temp_1_preset_index = 0;
            }

            save_set_values();
            
            state = STATE_DISPLAY;
            delay(10); // debounce
            break;

        case STATE_DISPLAY:
            // show blinking dot
            oled.set_cursor(0, 7, SMALL_FONT);
            if (show_dot == 1) {
                show_dot = 0;
                oled.write(" ");
            }
            else {
                show_dot = 1;
                oled.write(".");
            }

            //---------------------------------------------------------------
            // 1 ON-OFF regulator
            //---------------------------------------------------------------
            // upper point
            if (hysteresis_1_raising && (current_temp_1 >= (set_temp_1 + UPPER_HYSTERESIS))) {
                hysteresis_1_raising = FALSE;
            }
            // lower point
            else if (!hysteresis_1_raising && (current_temp_1 <= (set_temp_1 - LOWER_HYSTERESIS))) {
                hysteresis_1_raising = TRUE;
            }
            // raising
            if (hysteresis_1_raising) {
                switch_relay(1, ON);
                switch_relay(2, ON);
            }
            // falling
            else {
                switch_relay(1, OFF);
                switch_relay(2, OFF);
            }

            //---------------------------------------------------------------
            // 2 ON-OFF regulator
            //---------------------------------------------------------------
            // upper point
            if (hysteresis_2_raising && (current_temp_2 >= (set_temp_2 + UPPER_HYSTERESIS))) {
                hysteresis_2_raising = FALSE;
            }
            // lower point
            else if (!hysteresis_2_raising && (current_temp_2 <= (set_temp_2 - LOWER_HYSTERESIS))) {
                hysteresis_2_raising = TRUE;
            }
            // raising
            if (hysteresis_2_raising) {
                switch_relay(3, ON);
                switch_relay(4, ON);
            }
            // falling
            else {
                switch_relay(3, OFF);
                switch_relay(4, OFF);
            }

            //---------------------------------------------------------------
            // Control high temp mode
            //---------------------------------------------------------------
            if (((millis() - high_temp_mode_start_time) >= HIGH_TEMP_ON_TIME) && (set_temp_1 == HIGH_TEMP)) {
                set_temp_1 = NORMAL_TEMP;
            }

            break;
    }
}
