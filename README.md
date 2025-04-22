# PicoW USB Audio to Bluetooth Adapter
The Pico W USB Audio to Bluetooth Adapter transforms your Raspberry Pi Pico W into a high-quality, hassle-free audio streaming device. 
It allows any device with a USB input, including MacOS, Windows, Linux, Nintendo Switch, PS4/PS5, and more, to gain Bluetooth Audio capability. 
With Pico W Adapter, you can easily transmit audio wirelessly from your USB audio source to your Bluetooth headphones or speaker, enhancing your listening experience.

To make sure we can keep working on free and open-source projects like this,  
**please consider becoming a [:heart: Sponsor via PayPal](https://www.paypal.com/donate/?business=UZAK3WFV233ML&no_recurring=0&item_name=Help+me+build+more+project%21&currency_code=USD) or support us via [:coffee: Ko-fi](https://ko-fi.com/wasdwasd0105).**  


<p align="center">
<img alt="Logo" src="logo.png" width="200">
</p>

### Driver-Free Setup
Setting up Pico W/2 W requires no driver or software installation. Simply plug the Pico W into your device's USB port, set your Bluetooth headphones or speakers to pairing mode, and an automatic connection will be made. 

### Bluetooth SBC Codec
Pico W Bluetooth Adapter utilizes the sbc codec to deliver high-quality audio. It can steam with stereo sound. 

### Video demo

[![video demo](http://img.youtube.com/vi/Dilagi7l4xc/0.jpg)](http://www.youtube.com/watch?v=Dilagi7l4xc "")


## Installation

Installing the Pico W USB Audio to Bluetooth Adapter firmware involves flashing a UF2 (USB Flashing Format) file onto your Raspberry Pi Pico. Follow these steps:

1. **Download the UF2 file:** You can find the latest firmware for the PicoW USB Audio to Bluetooth Adapter at the [releases page](https://github.com/wasdwasd0105/PicoW-usb2bt-audio/releases) of the GitHub repository. Download the `.uf2` file from the latest release. 

2. **Connect the Pico to your computer:** First, ensure that your Pico is not connected to your computer. Then, hold down the 'BOOTSEL' button on the Pico while you plug it into your computer using a micro USB cable. It will appear on your computer as a mass storage device (like a USB flash drive).

3. **Copy the UF2 file:** Simply drag and drop (or copy and paste) the downloaded UF2 file onto the Pico. 

4. **Reset the Pico:** Once the UF2 file has been copied onto the Pico, it will automatically reset and start running the new firmware. 



## Usage

Using the PicoW USB Audio to Bluetooth Adapter is a straightforward process. Here are the steps to follow:

1. **Connect your Pico W to your audio source device:** Use a USB cable to connect your Raspberry Pi Pico W to the device that you want to stream audio from.

2. **Set the audio output on your source device:** On your audio source device, go to your sound settings and change the audio output device to `USB Bluetooth Audio`. 

3. **Pairing a new device:** To pair a new device, long press the 'BOOTSEL' button on the Pico W and release it the led light will blink fast. Then, put the new Bluetooth device into pairing mode. The Pico W will automatically connect to it.

4. **Start playing audio:** Once everything is set up, you can start playing audio from your source device. The audio will be streamed to your Bluetooth device via the Pico W.

5. **Reconnecting a device:** You can reconnect the headphone by short pressing the 'BOOTSEL' button.



## Green LED light Status Indicator

1. **Blinking Slow (1s):** When the Green LED light is blinking, it indicates that audio is currently streaming.
2. **Blinking Fast (0.5s):** It means that the PicoW Adapter is in pairing mode.
3. **On (Steady Light):** It means that the PicoW Adapter is on standby. Short-press the key to reconnect the last saved device.


## Compile & Debug

In order to compile the PicoW USB Audio to Bluetooth Adapter firmware from source code, you need to follow these steps:

1. **Prepare your environment:** Use VS Code and Raspberry Pi Pico extension

2. **Import the project on the Raspberry Pi Pico extension:** Import the project using General -> Import Project

Choose Pico W or Pico 2 W using Switch Board

Then you can Debug, Compile and Run the project on the Project tab

3. **Debug Serial input/output:** You can use uart to see the debug info. Connect the GPIO 0 and 1 as TX and RX. To enable BTstack's serial input, you can uncomment `HAVE_BTSTACK_STDIN` under btstack_config.h
    

## Acknowledgments
1. [usb-sound-card](https://github.com/hathach/tinyusb/tree/master/examples/device/uac2_headset): Tinyusb uac2 demo.

2. [a2dp_source_demo](https://github.com/bluekitchen/btstack/blob/master/example/a2dp_source_demo.c): The Advanced Audio Distribution Profile (A2DP) source demo provided by the BTstack.



## License

This project is licensed under the terms of the Apache License 2.0.

