# PicoW USB Bluetooth Audio Adapter
Use Raspberry Pico W as USB to Bluetooth Audio Adapter

## Install 
Download the .uf2 firmware, and use this [guide](https://www.okdo.com/getting-started/get-started-with-raspberry-pi-pico/) to install.

## Usage
1. Plug in your Pico W to your audio source device. 
2. Choose the audio source device output to `USB Bluetooth Audio`
3. Put the Bluetooth speakers/headphones on pairing mode. Raspberry Pico W will make auto-connect.
4. Play some music. If the sound has noise. Short press the key for times until the sound is clear.
5. Long press the key and release to disconnect and pair a new device.

## Green LED light
1. Blinking: streaming audio
2. On: pairing (inaccurate)

## Compile
0. Make sure you know how to compile Pico example projects
1. install the compiler 
2. export `PICO_SDK_PATH` and `PICO_EXTRAS_PATH` path.
3. `./build.sh`
