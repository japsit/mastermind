// Based on an ViOpe's example in chapter 17.8.
#ifndef BUTTONS_H
#define BUTTONS_H

#define BUTTON1_DOWN (!(PINA & (1 << PA0)))
#define BUTTON2_DOWN (!(PINA & (1 << PA1)))
#define BUTTON3_DOWN (!(PINA & (1 << PA2)))
#define BUTTON4_DOWN (!(PINA & (1 << PA3)))
#define BUTTON5_DOWN (!(PINA & (1 << PA4)))

#endif