TARGET=hw5bryan

SOURCES=import_registers.c \
        enable_pwm_clock.c \
        hw5bryan.c

OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

all: $(OBJECTS)
	gcc $(OBJECTS) -lpthread -lpigpio -lrt -o $(TARGET)

clean:
	rm -f $(OBJECTS) $(TARGET)

%.o:%.c
	gcc -c $< -o $@
