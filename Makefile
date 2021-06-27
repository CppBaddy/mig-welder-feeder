CC=avr-g++

DEVICE=85

#Signature
#ATtiny25 0x1E 0x91 0x08
#ATtiny45 0x1E 0x92 0x06
#ATtiny85 0x1E 0x93 0x0B

#Fuses
EFUSE=0xff
HFUSE=0xdf
LFUSE=0x62

#E:FE, H:DD, L:E1

CFLAGS=-g -std=gnu++11 -Os -Wall -mcall-prologues -mmcu=attiny$(DEVICE) -DF_CPU=1000000
## Use short (8-bit) data types
CFLAGS += -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums

LDFLAGS = -Wl,-Map,$(TARGET).map
## Optional, but often ends up with smaller code
LDFLAGS += -Wl,--gc-sections

OBJ2HEX=avr-objcopy

SIZE=avr-size

UISP=avrdude

TARGET=mig-welder-feeder

C_FILES = $(wildcard *.c*)

OBJS = $(C_FILES:.cpp=.o)

all : build

program : flash

fuse :
	$(UISP) -p t$(DEVICE) -c USBasp -v -U hfuse:w:${HFUSE}:m -U lfuse:w:${LFUSE}:m 

flash : $(TARGET).hex
	$(UISP) -p t$(DEVICE) -c USBasp -v -U flash:w:$(TARGET).hex:i

build : $(TARGET).hex
	ls -l $(TARGET).*

%.hex : %.elf
	$(OBJ2HEX) -O ihex $< $@

%.elf : $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) -o $@
	$(SIZE) -t $@

%.o : %.cpp Makefile
	$(CC) $(CFLAGS) -c $< -o $@

clean :
	@rm -f *.hex *.eep *.elf *.o

help :
	@echo "make [help | clean | build | eeprom | flash | program | fuse]"
