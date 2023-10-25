# 3keys_1knob
Custom firmware for a 3-key + rotary encoder macropad.

Forked from [biemster/3keys_1knob](https://github.com/biemster/3keys_1knob/), which is based on [wagiminator/CH552-Macropad-mini](https://github.com/wagiminator/CH552-Macropad-mini/).

Compared to original, keypresses are generated on key release, to allow key combinations.

Also, layer and combination support is added, and sending multimedia (consumer) keycodes.

## Instructions

### compile:
`$ make bin`

### compile & flash to pad:
- if on original firmware: connect P1.5 to GND and connect USB
	- alternate method, on some boards: USB- to 3.3V, using 10k resistor
- if on this firmware: press key1 while connecting USB
	- alternate method: press and hold all three keys
- `$ make flash`

### configure keys:
1. `$ make get_isp` (first time only)
2. `$ make dump`
3. edit `flashdata.bin` (for example with `hexedit`)
4. `$ make data`

## Flash Map

| Position | Layer | Key 1  | Key 2  | Key 3  | Encoder Switch | Encoder CW | Encoder CCW | Foreground | Max layers |
|----------|-------|--------|--------|--------|----------------|------------|-------------|------------|------------|
| ` 0-15 ` | `0`   | `MMCC` | `MMCC` | `MMCC` | `MMCC`         | `MMCC`     | `MMCC`      | `RRGGBB`   | `NN`       |

| Position | Layer | Key 1+2 | Key 2+3 | Key 1+3 | Encoder pressed CW | Encoder pressed CCW | Fade     | Encoder pressed CW | Background | Encoder pressed CCW |
|----------|-------|---------|---------|---------|--------------------|---------------------|----------|--------------------|------------|---------------------|
| `16-32`   | `0`   | `MMCC`  | `MMCC`  | `MMCC`  | `CC`               | `CC`                | `RRGGBB` | `MM`               | `RRGGBB`   | `MM`                |

Layers 1, 2, 3 and have the same layout. Layer 1 starts at 33, and so on.
`Max layers` exist only on layer 0. On other, this value is reserved for future use.

### Meanings

- `MMCC` - four bytes, key settings
	- `MM` - modifier:
		- if set to `0xFF`, character code will be sent as Consumer Keyboard Keycode
		- otherwise, bits in order: `(7) RG RA RS RC LG LA LS LC (0)`
		- `R` - right, `L` - left, `C` - ctrl, `S` - shift, `A` - alt, `G` - gui (win)
	- `CC` - keycode (ASCII, or from `usb_conkbd.h`)
- `MM` and `CC` - encoder pressed settings are split into two halves
	- this indicates settings for when encoder is pressed down and then turned
	- it is active only when layers are disabled (`max_layers` set to `0`)
- `RRGGBB` - 24-bit colour, byte for red, green and blue:
	- foreground - layer colour, used when switching, and after switch has been pressed,
	- background - default level, foreground will fade out to it,
	- fade - fade step, each loop removes these values from current level, until background level is reached,
- `max layers` - `NN` - `0-3` controls keyboard behaviour, as explained below.
	
### Layers and sequences

As data memory is very limited, some tradeoffs had to be made.

This keyboard can operate in four different modes, as selected by `max layers` value.

- `0` - only layer `0` is active, but each key can use up to four keys in sequence.
	- first key from layer `0` will be pressed, then from `1`, `2`, and `3`
- `1` - layers `0` and `3` will be active. Up to three keys on layer `0`, and one on `3`.
	- layer `0` will press keys from `0`, `1`, and `2`
- `2` - layers `0`, `2`, and `3` active, layer `0` uses keys from `0` and `1`, in sequence.
- `3` - all layers are active, no sequences available, only one keypress keypress.

To switch between layers:
	- press and hold encoder's switch to switch to layer `0`
	- press and rotate encoder to switch to other layers
