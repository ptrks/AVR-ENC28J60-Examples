DUDECPUTYPE = m328p
MCU         = atmega328p
PROGRAMMER  = usbasp
PORT        = /dev/ttyACM0

INCDIR = -I../../tuxlib
LOADCMD     = avrdude -p $(DUDECPUTYPE) -c $(PROGRAMMER) -e -U flash:w:main.hex

CC=avr-gcc
OBJCOPY=avr-objcopy
# optimize for size:
CFLAGS=-g -mmcu=$(MCU) $(INCDIR) -Wall -W -Os -mcall-prologues
#-------------------
.PHONY: all main
#
all: main.hex 
	@echo "done"
#
main: main.hex
	@echo "done"
#
#-------------------
help: 
	@echo "Usage: make all|main.hex"
	@echo "or"
	@echo "make fuse|rdfuses"
	@echo "or"
	@echo "make load"
	@echo "or"
	@echo "Usage: make clean"
	@echo " "
	@echo "You have to set the low fuse byte to 0x60 on all new tuxgraphics boards".
	@echo "This can be done with the command (linux/mac if you use avrusb500): make fuse"
#-------------------
main.hex: main.elf 
	$(OBJCOPY) -R .eeprom -O ihex main.elf main.hex 
	avr-size main.elf
	@echo " "
	@echo "Expl.: data=initialized data, bss=uninitialized data, text=code"
	@echo " "

main.elf: main.o ip_arp_udp_tcp.o enc28j60.o websrv_help_functions.o
	$(CC) $(CFLAGS) -o main.elf -Wl,-Map,main.map main.o ip_arp_udp_tcp.o enc28j60.o websrv_help_functions.o
websrv_help_functions.o: ../../tuxlib/websrv_help_functions.c ../../tuxlib/websrv_help_functions.h ../../tuxlib/ip_config.h 
	$(CC) $(CFLAGS) -Os -c ../../tuxlib/websrv_help_functions.c
enc28j60.o: ../../tuxlib/enc28j60.c ../../tuxlib/timeout.h ../../tuxlib/enc28j60.h
	$(CC) $(CFLAGS) -Os -c ../../tuxlib/enc28j60.c
ip_arp_udp_tcp.o: ../../tuxlib/ip_arp_udp_tcp.c ../../tuxlib/net.h ../../tuxlib/enc28j60.h ../../tuxlib/ip_config.h
	$(CC) $(CFLAGS) -Os -c ../../tuxlib/ip_arp_udp_tcp.c
main.o: main.c ../../tuxlib/ip_arp_udp_tcp.h ../../tuxlib/enc28j60.h ../../tuxlib/timeout.h ../../tuxlib/net.h ../../tuxlib/websrv_help_functions.h ../../tuxlib/ip_config.h
	$(CC) $(CFLAGS) -Os -c main.c
#------------------
load: main.hex
	$(LOADCMD)
#-------------------
clean:
	rm -f *.o *.map *.elf  main.hex
#-------------------
