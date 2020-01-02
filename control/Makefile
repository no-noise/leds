HEADERS :=		control.h network.h panels.h shared.h
SOURCES :=		control.ino network.cpp panels.cpp shared.cpp

BUILD :=		$(shell pwd)/build

FQBN :=			esp32:esp32:esp32
PORT1 :=		/dev/ttyUSB0
PORT2 :=		/dev/ttyUSB1
SPEED :=		115200

BASE :=			control.esp32.esp32.esp32

# Don't enable additional warnings via command line flags; include warnings.h
# instead. Arduino headers produce too many warnings.
EXTRA :=		-fno-strict-aliasing -fno-inline -fno-omit-frame-pointer

all:			$(BASE).bin

$(BASE).bin:	$(HEADERS) $(SOURCES)
				arduino-cli compile --build-path $(BUILD) \
					--fqbn $(FQBN) --warnings all \
					--build-properties "compiler.c.extra_flags=$(EXTRA)" \
					--build-properties "compiler.cpp.extra_flags=$(EXTRA)"

upload1:		$(BASE).bin
				arduino-cli upload --fqbn $(FQBN) --port $(PORT1)

upload2:		$(BASE).bin
				arduino-cli upload --fqbn $(FQBN) --port $(PORT2)

serial1:
				cu -l $(PORT1) -s $(SPEED)

serial2:
				cu -l $(PORT2) -s $(SPEED)

run1:			upload1 serial1

run2:			upload2 serial2

clean:
				rm -f $(BASE).bin $(BASE).partitions.bin $(BASE).elf
				rm -rf $(BUILD)

update:
				arduino-cli lib update-index
				arduino-cli lib upgrade
				arduino-cli core update-index
				arduino-cli core upgrade
