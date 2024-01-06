SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

EXECUTABLE_PING := exe_ping
EXECUTABLE_PONG := exe_pong
ALL_EXECUTABLES := \
	$(EXECUTABLE_PING) \
	$(EXECUTABLE_PONG)

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: all clean

all: $(ALL_EXECUTABLES)

$(EXECUTABLE_PING): local_ping.o
	$(CC) $(CFLAGS) $< -o $@

$(EXECUTABLE_PONG): local_pong.o
	$(CC) $(CFLAGS) $< -o $@

clean: $(ALL_EXECUTABLES) $(OBJ)
	$(RM) $^
