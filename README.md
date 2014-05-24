AVR TCP/IP Examples
===================

In this repo you will find ready to go examples that allow you to get your microcontroller connected to the internet immediately. All of the code examples have been tested with the ATmega328p and ATmega128. In order to achieve TCP/IP capabilities we will be making use of the wonderful [tuxgraphics TCP/IP stack](http://tuxgraphics.org/electronics/200905/embedded-tcp-ip-stack.shtml).

###Prerequisites
Besides having an ATmega328p or ATmega128 microcontroller these tutorials assume you are using the [Microchip ENC28J60](http://www.microchip.com/wwwproducts/Devices.aspx?dDocName=en022889)  ethernet controller module. You can pick this module up fairly cheaply at a variety of places:

* [Amazon (This is the one I actually bought and used)](http://www.amazon.com/ENC28J60-Ethernet-Network-Module-STM32/dp/B008B4QIV2)
* [Ebay](http://www.ebay.com/sch/i.html?_trksid=p2050601.m570.l1311.R1.TR10.TRC2.A0.H0.Xenc28j&_nkw=enc28j60&_sacat=0&_from=R40)
* [AliExpress](http://www.aliexpress.com/w/wholesale-enc28j60.html)



###Getting Started
Hooking up the ENC28J260 module to your micocontroller is very straight forward:

| Pin Function  | ATmega328p    | ATmega128   | Arduino UNO & Duemilanove |
|:---------------:|:---------------:|:------------:|:-----------------------------------:|
| CS    | PB2   | PB0  | Digital Pin 10     |
| MOSI  | PB3   | PB2  | Digital Pin 11     |
| MISO   | PB4   | PB3  | Digital Pin 12    |
| SCK   | PB5   | PB1  | Digital Pin 13     |


###Loading An Example
If you haven't noticed yet there are two directories of code. The tuxlib directory has all of the tuxgraphics TCP/IP library source code. The examples directory will have multiple examples and individual makefiles for each. Before you compile and load the program you may need to make some changes to the `makefile` that is in the example folder. In the `makefile` you can change:
* Target microcontroller type - `DUDECPUTYPE` and `MCU`
* Programmer - `PROGRAMMER`
* The port your programmer is using - `PORT`

After you modify the `makefile` compilation and installation are easy. From the console run:
* `make` to compile the source code of the example
* `make load` to load the code onto your microcontroller

Because most of these examples are based on the tuxgraphics library make sure to check out [their documentation as well](http://tuxgraphics.org/electronics/200905/embedded-tcp-ip-stack.shtml).