// ===================================================================================
// Project:   MacroPad Mini for CH551, CH552 and CH554
// Version:   v1.1
// Year:      2023
// Author:    Stefan Wagner
// Github:    https://github.com/wagiminator
// EasyEDA:   https://easyeda.com/wagiminator
// License:   http://creativecommons.org/licenses/by-sa/3.0/
// ===================================================================================
//
// Description:
// ------------
// Firmware example implementation for the MacroPad Mini.
//
// References:
// -----------
// - Blinkinlabs: https://github.com/Blinkinlabs/ch554_sdcc
// - Deqing Sun: https://github.com/DeqingSun/ch55xduino
// - Ralph Doncaster: https://github.com/nerdralph/ch554_sdcc
// - WCH Nanjing Qinheng Microelectronics: http://wch.cn
//
// Compilation Instructions:
// -------------------------
// - Chip:  CH551, CH552 or CH554
// - Clock: min. 12 MHz internal
// - Adjust the firmware parameters in include/config.h if necessary.
// - Make sure SDCC toolchain and Python3 with PyUSB is installed.
// - Press BOOT button on the board and keep it pressed while connecting it via USB
//   with your PC.
// - Run 'make flash'.
//
// Operating Instructions:
// -----------------------
// - Connect the board via USB to your PC. It should be detected as a HID keyboard.
// - Press a macro key and see what happens.
// - To enter bootloader hold down key 1 while connecting the MacroPad to USB. All
//   NeoPixels will light up white as long as the device is in bootloader mode 
//   (about 10 seconds).


// ===================================================================================
// Libraries, Definitions and Macros
// ===================================================================================

// Libraries
#include <config.h>                         // user configurations
#include <system.h>                         // system functions
#include <delay.h>                          // delay functions
#include <neo.h>                            // NeoPixel functions
#include <usb_conkbd.h>                     // USB HID consumer keyboard functions

// Prototypes for used interrupts
void USB_interrupt(void);
void USB_ISR(void) __interrupt(INT_NO_USB) {
  USB_interrupt();
}

// ===================================================================================
// NeoPixel Functions
// ===================================================================================

enum Event {
  NONE,
  KEY1,
  KEY2,
  KEY3,
  ENC_SW,
  ENC_CW,
  ENC_CCW,
  KEY12,
  KEY23,
  KEY13,
  ENC_SW_CW,
  ENC_SW_CCW
};

struct RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct Chars {
  char mod1;
  char char1;
  char mod2;
  char char2;
  char mod3;
  char char3;
  char modSW;
  char charSW;
  char modCW;
  char charCW;
  char modCCW;
  char charCCW;
  char mod12;
  char char12;
  char mod23;
  char char23;
  char mod13;
  char char13;
  char swcharCW;
  char swcharCCW;
  char swmodCW;
  char swmodCCW;
};

__idata struct RGB neo[4];
__idata struct RGB neobg[4];
__idata struct RGB neofg[4];
__idata struct RGB neofade[4];

__idata uint8_t layer = 0;
__idata uint8_t max_layer = 0;
__idata struct Chars chars[4];

__idata const int8_t encoder_factor[] = { 0, 1, -1, 2, -1, 0, -2, 1, 1, -2, 0, -1, 2, -1, 1, 0 };

// Update NeoPixels
void NEO_update(void) {
  EA = 0;                                   // disable interrupts
  NEO_writeColor(neo[1].r, neo[1].g, neo[1].b);
  NEO_writeColor(neo[2].r, neo[2].g, neo[2].b);
  NEO_writeColor(neo[3].r, neo[3].g, neo[3].b);
  EA = 1;                                   // enable interrupts
}

void set_neo_rgb(uint8_t n, uint8_t r, uint8_t g, uint8_t b);
void set_neo_rgb(uint8_t n, uint8_t r, uint8_t g, uint8_t b) {
  if (n == 0) {
    set_neo_rgb(1, r, g, b);
    set_neo_rgb(2, r, g, b);
    set_neo_rgb(3, r, g, b);
  } else { neo[n].r = r; neo[n].g = g; neo[n].b = b; }
}

void set_neo_fg(uint8_t n);
void set_neo_fg(uint8_t n) {
  if (n == 0) { set_neo_fg(1); set_neo_fg(2); set_neo_fg(3); }
  else { set_neo_rgb(n, neofg[layer].r, neofg[layer].g, neofg[layer].b); }
}

void set_neo_bg(uint8_t n);
void set_neo_bg(uint8_t n) {
  if (n == 0) { set_neo_bg(1); set_neo_bg(2); set_neo_bg(3); }
  else { set_neo_rgb(n, neobg[layer].r, neobg[layer].g, neobg[layer].b); }
}

uint8_t safe_fade(uint8_t from, uint8_t by, uint8_t min) {
  if (from <= min) { return min; }
  if (from < by) { return min; }
  from -= by;
  if (by < min) { return min; }
  return from;
}

void fade_out(uint8_t n);
void fade_out(uint8_t n) {
  if (n == 0) { fade_out(1); fade_out(2); fade_out(3); }
  else {
    neo[n].r = safe_fade(neo[n].r, neofade[layer].r, neobg[layer].r);
    neo[n].g = safe_fade(neo[n].g, neofade[layer].g, neobg[layer].g);
    neo[n].b = safe_fade(neo[n].b, neofade[layer].b, neobg[layer].b);
  }
}

void type_uint(uint8_t n) {
  KBD_type('0' + (n / 100));
  KBD_type('0' + ((n % 100) / 10));
  KBD_type('0' + ((n % 10) / 1));
}

void type_delimit() { KBD_type(','); KBD_type(' '); }

void parse_layer(int8_t dir) {
  if (max_layer > 3) { max_layer = 3; }
  if (dir == 0) { layer = 0; return; }
  switch (max_layer) {
    case 0:
      layer = 0;
      break;
    case 1:
      layer = 3;
      break;
    case 2:
      if (dir  > 0) { layer = 2; return; }
      if (dir  < 0) { layer = 3; return; }
      break;
    case 3:
      layer += dir;
      if (layer > 3) { layer = 1; }
      if (layer < 1) { layer = 3; }
      break;
  }
}

void mod_type(char c, uint8_t mod);
void get_type(enum Event ev, uint8_t n);
void parse_type(enum Event ev) {
  get_type(ev, layer);
  if ((layer == 0) && (max_layer < 3)) {
    if (max_layer <= 2) { get_type(ev, 1); }
    if (max_layer <= 1) { get_type(ev, 2); }
    if (max_layer <= 0) { get_type(ev, 3); }
  }
}

void get_type(enum Event ev, uint8_t n) {
  char c = 0;
  uint8_t mod = 0;
  switch (ev) {
    case KEY1:
      c = chars[n].char1; mod = chars[n].mod1; break;
    case KEY2:
      c = chars[n].char2; mod = chars[n].mod2; break;
    case KEY3:
      c = chars[n].char3; mod = chars[n].mod3; break;
    case ENC_SW:
      c = chars[n].charSW; mod = chars[n].modSW; break;
    case ENC_CW:
      c = chars[n].charCW; mod = chars[n].modCW; break;
    case ENC_CCW:
      c = chars[n].charCCW; mod = chars[n].modCCW; break;
    case KEY12:
      c = chars[n].char12; mod = chars[n].mod12; break;
    case KEY23:
      c = chars[n].char23; mod = chars[n].mod23; break;
    case KEY13:
      c = chars[n].char13; mod = chars[n].mod13; break;
    case ENC_SW_CW:
      c = chars[n].swcharCW; mod = chars[n].swmodCW; break;
    case ENC_SW_CCW:
      c = chars[n].swcharCCW; mod = chars[n].swmodCCW; break;
  }
  if (c == 0) { return; }
  if (mod == 0xFF) { CON_type(c); return; }
  mod_type(c, mod);
}

void mod_type(char c, uint8_t mod) {
  if (mod & 1) { KBD_press(KBD_KEY_LEFT_CTRL); }
  if (mod & 2) { KBD_press(KBD_KEY_LEFT_SHIFT); }
  if (mod & 4) { KBD_press(KBD_KEY_LEFT_ALT); }
  if (mod & 8) { KBD_press(KBD_KEY_LEFT_GUI); }
  if (mod & 16) { KBD_press(KBD_KEY_RIGHT_CTRL); }
  if (mod & 32) { KBD_press(KBD_KEY_RIGHT_SHIFT); }
  if (mod & 64) { KBD_press(KBD_KEY_RIGHT_ALT); }
  if (mod & 128) { KBD_press(KBD_KEY_RIGHT_GUI); }
  KBD_type(c);
  if (mod & 128) { KBD_release(KBD_KEY_RIGHT_GUI); }
  if (mod & 64) { KBD_release(KBD_KEY_RIGHT_ALT); }
  if (mod & 32) { KBD_release(KBD_KEY_RIGHT_SHIFT); }
  if (mod & 16) { KBD_release(KBD_KEY_RIGHT_CTRL); }
  if (mod & 8) { KBD_release(KBD_KEY_LEFT_GUI); }
  if (mod & 4) { KBD_release(KBD_KEY_LEFT_ALT); }
  if (mod & 2) { KBD_release(KBD_KEY_LEFT_SHIFT); }
  if (mod & 1) { KBD_release(KBD_KEY_LEFT_CTRL); }
}

// Read EEPROM (stolen from https://github.com/DeqingSun/ch55xduino/blob/ch55xduino/ch55xduino/ch55x/cores/ch55xduino/eeprom.c)
uint8_t eeprom_read_byte (uint8_t addr){
  ROM_ADDR_H = DATA_FLASH_ADDR >> 8;
  ROM_ADDR_L = addr << 1; //Addr must be even
  ROM_CTRL = ROM_CMD_READ;
  return ROM_DATA_L;
}

// ===================================================================================
// Main Function
// ===================================================================================
void enter_bootloader(void) {
  __idata uint8_t i;                        // temp variable
  NEO_latch();                            // make sure pixels are ready
  for(i=9; i; i--) NEO_sendByte(127);     // light up all pixels
  BOOT_now();                             // enter bootloader
}

void main(void) {
  // Variables
  __idata uint8_t i = 0;
  __idata uint8_t n = 0;
  __bit key1 = 0;
  __bit key2 = 0;
  __bit key3 = 0;
  __bit keyenc = 0;
  __bit warning = 0;
  __idata uint8_t press = 0;
  __idata uint8_t hold = 0;
  __idata uint8_t all = 0;  // three keys held
  __idata uint8_t knob = 0; // knob held down
  __idata uint8_t show_mode = 0;
  __bit mode_changed = 0;
  __idata uint8_t encoder_state = 0;
  __idata int8_t encoder_value = 0;
  __idata int8_t encoder_dir = 0;
  // __idata struct RGB neomode;

  NEO_init();
  if (!PIN_read(PIN_KEY1)) { enter_bootloader(); }

  CLK_config(); DLY_ms(5); KBD_init(); WDT_start();

  for (i = 0; i <= 3; i++) {
    n = i * 32;
    chars[i].mod1 = (char)eeprom_read_byte(n++);
    chars[i].char1 = (char)eeprom_read_byte(n++);
    chars[i].mod2 = (char)eeprom_read_byte(n++);
    chars[i].char2 = (char)eeprom_read_byte(n++);

    chars[i].mod3 = (char)eeprom_read_byte(n++);
    chars[i].char3 = (char)eeprom_read_byte(n++);
    chars[i].modSW = (char)eeprom_read_byte(n++);
    chars[i].charSW = (char)eeprom_read_byte(n++);

    chars[i].modCW = (char)eeprom_read_byte(n++);
    chars[i].charCW = (char)eeprom_read_byte(n++);
    chars[i].modCCW = (char)eeprom_read_byte(n++);
    chars[i].charCCW = (char)eeprom_read_byte(n++);

    neofg[i].r = eeprom_read_byte(n++);
    neofg[i].g = eeprom_read_byte(n++);
    neofg[i].b = eeprom_read_byte(n++);
    max_layer = eeprom_read_byte(n++);

    chars[i].mod12 = (char)eeprom_read_byte(n++);
    chars[i].char12 = (char)eeprom_read_byte(n++);
    chars[i].mod23 = (char)eeprom_read_byte(n++);
    chars[i].char23 = (char)eeprom_read_byte(n++);

    chars[i].mod13 = (char)eeprom_read_byte(n++);
    chars[i].char13 = (char)eeprom_read_byte(n++);
    chars[i].swcharCW = (char)eeprom_read_byte(n++);
    chars[i].swcharCCW = (char)eeprom_read_byte(n++);

    neofade[i].r = eeprom_read_byte(n++);
    neofade[i].g = eeprom_read_byte(n++);
    neofade[i].b = eeprom_read_byte(n++);
    chars[i].swmodCW = (char)eeprom_read_byte(n++);

    neobg[i].r = eeprom_read_byte(n++);
    neobg[i].g = eeprom_read_byte(n++);
    neobg[i].b = eeprom_read_byte(n++);
    chars[i].swmodCCW = (char)eeprom_read_byte(n++);

    if ((neofg[i].r | neofg[i].g | neofg[i].b) == 0) {
      neofg[i].r = 0xFF; neofg[i].g = 0x16;
    }
    if ((neobg[i].r | neobg[i].g | neobg[i].b) == 0) {
      neobg[i].r = 0xC; neobg[i].g = 0x1;
    }
    if (neofade[i].r == 0) { neofade[i].r = 1; }
    if (neofade[i].g == 0) { neofade[i].g = 1; }
    if (neofade[i].b == 0) { neofade[i].b = 1; }
  }

  if ((chars[0].char1 | chars[0].char2 | chars[0].char3) == 0) {
    neo[1].r = 255; neo[1].g = 0; neo[1].b = 0; NEO_update();
    DLY_ms(200); neo[1].r = 0; NEO_update();
    DLY_ms(200); neo[1].r = 255; NEO_update();
    DLY_ms(200); neo[1].r = 0; NEO_update();
    DLY_ms(200); neo[1].r = 255; NEO_update();
    DLY_ms(200);
  }
  set_neo_fg(1); set_neo_fg(2); set_neo_fg(3);

  while (1) {
    if (!PIN_read(PIN_KEY1) != key1) { key1 = !key1; }
    if (!PIN_read(PIN_KEY2) != key2) { key2 = !key2; }
    if (!PIN_read(PIN_KEY3) != key3) { key3 = !key3; }

    hold = 0;
    if (key1) { hold |= 1; press |= 1; set_neo_fg(1); }
    if (key2) { hold |= 2; press |= 2; set_neo_fg(2); }
    if (key3) { hold |= 4; press |= 4; set_neo_fg(3); }

    if (hold == 0) {
      switch (press) {
        case 1: parse_type(KEY1); break;
        case 2: parse_type(KEY2); break;
        case 4: parse_type(KEY3); break;
        case 3: parse_type(KEY12); break;
        case 5: parse_type(KEY13); break;
        case 6: parse_type(KEY23); break;
      }
      press = 0;
      all = 0;
    } else {
        if (hold == 7) { all++; if (all > 200) { enter_bootloader(); } }
    }

    if (!PIN_read(PIN_ENC_SW) != keyenc) {
      keyenc = !keyenc;
      if (keyenc) { mode_changed = 0; }
      if (!keyenc && !mode_changed) { parse_type(ENC_SW); }
    }

    i = (encoder_state & 3) | ((!PIN_read(PIN_ENC_A)) << 2) | ((!PIN_read(PIN_ENC_B)) << 3);
    encoder_state = i >> 2;
    encoder_value += encoder_factor[i];
    encoder_dir = 0;
    if (encoder_value >= 4)  { encoder_dir = -1; encoder_value -= 4; }
    if (encoder_value <= -4) { encoder_dir =  1; encoder_value += 4; }

    if (keyenc) { // encoder pressed
      if (encoder_dir) {
        mode_changed = 1;
        if (max_layer > 0) {
          parse_layer(encoder_dir);
          show_mode = 60;
        } else {
          if (encoder_dir > 0) { parse_type(ENC_SW_CCW); }
          if (encoder_dir < 0) { parse_type(ENC_SW_CW); }
        }
      }
      if (max_layer > 0) {
        if (knob > 200) { parse_layer(0); mode_changed = 1; }
        if (mode_changed) { show_mode = 60; knob = 0; } else { knob++; }
      }
    } else {
      if (encoder_dir > 0) { parse_type(ENC_CCW); }
      if (encoder_dir < 0) { parse_type(ENC_CW); }
      knob = 0;
    }

    NEO_update();
    fade_out(0);
    if (show_mode) {
      show_mode--;
      set_neo_rgb(0, neofg[layer].r, neofg[layer].g, neofg[layer].b);
    }

    DLY_ms(5);
    WDT_reset();
  }
}
