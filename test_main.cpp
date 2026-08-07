#include "Converter.h"
#include <linux/input-event-codes.h>
#include <iostream>

int main() {
    Converter c;
    c.conv_key = 0;
    c.ls_keys[0] = 56; // Alt
    c.ls_keys[1] = 42; // Shift

    // type "hello"
    for (char ch : std::string("hello")) {
        c.push(ch - 'a' + KEY_A, 1);
    }
    // hold LeftShift, double-press RightShift (5-event pattern, first shift still held)
    c.push(KEY_LEFTSHIFT, 1);
    c.push(KEY_RIGHTSHIFT, 1);
    c.push(KEY_RIGHTSHIFT, 0);
    c.push(KEY_RIGHTSHIFT, 1);
    c.push(KEY_RIGHTSHIFT, 0);

    std::cout << "Buffer before: " << c.get_buffer_dump() << "\n";
    Action a = c.process();
    std::cout << "Action: " << a << "\n";
    if (a != None) {
        std::cout << "Buffer after trim: " << c.get_buffer_dump() << "\n";
        auto evs = c.convert(a);
        for (auto &e : evs) {
            std::cout << "  code=" << e.code << " val=" << e.value << "\n";
        }
    }
    return 0;
}
