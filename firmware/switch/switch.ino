/*
Copyright 2012 Andres Vahter

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdlib.h>
#include "OLED.h"

OLED oled(13, 0x3C);

void setup() {
    oled.init();
    oled.clear();
    oled.set_cursor(7, 4, LARGE_FONT);
      
    oled.write(" SWITCH");  
    //oled.set_cursor(7, 4, LARGE_FONT);
    //oled.write("Term %i", -1);

    /*
    delay(1000);   
    delay(1000);
    delay(1000);
    delay(1000);
    
    oled.clear();
    oled.set_cursor(0,7,SMALL_FONT);
    oled.write(" T H E  T E R M I N A L");     
    oled.set_cursor(0,3,SMALL_FONT);
    oled.write("  su mo tu we th fr sa");
    */
}

void loop() {

}