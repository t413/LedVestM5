# LedVestM5
M5StickC-powered GIF-playing LED vest with TFT Preview

> [<img alt="preview gif2" src="https://thumbs.gfycat.com/VelvetyMediocreIrishwaterspaniel-size_restricted.gif" width="200" />](https://gfycat.com/VelvetyMediocreIrishwaterspaniel)
> <img alt="vest demo back" src="https://user-images.githubusercontent.com/326829/85434551-10eeba00-b53b-11ea-878d-2a63c2c5402f.jpg" width="200" />
> <img alt="vest demo lcd" src="https://user-images.githubusercontent.com/326829/85434554-12b87d80-b53b-11ea-952d-a08c20d63c29.jpg" width="200" />
> [<img alt="preview gif" src="https://thumbs.gfycat.com/HugeCarefulAldabratortoise-size_restricted.gif" height="270" />](https://gfycat.com/hugecarefulaldabratortoise)
>
> <sub>_(images clickable for much nicer video quality mpg versions)_</sub>

Uses the insane $10 [M5StickC](https://m5stack.com/products/stick-c) to drive a **16x16 LED matrix** and two strings of LEDs. 
Created for a friend's burning man LED vest: the matrix on the back and two strings of color highlights on the front.

Having a little preview screen control module for an LED matrix is _the best_. No more "I love that" from a random stranger without knowing what they're talking about.

1. The GIFs are saved to the internal esp32 storage (called SPIFFS) and easily uploaded. 
1. Everything _can_ be powered by the internal battery but **really** shouldn't be. 
1. Easily powered via the USB-C port on the bottom and connected with the output pins on top.
1. Brightness control (of all LEDs and display), manual or auto play, and next gif are controlled by buttons.
1. The front LEDs are _color-reactive_ to the currently playing gif!
1. The autoplay mode fades in&out so that the display is never jarring and awful to look at.

### Check out the [releases](https://github.com/t413/LedVestM5/releases) for compiled binaries **_and the 16x16 gifs I created!_**
