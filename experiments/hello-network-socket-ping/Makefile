SRC := $(wildcard *.c)
OBJ := $(SRC:.c=.o)

EXECUTABLE_PING := exe_ping
EXECUTABLE_PONG := exe_pong
ALL_EXECUTABLES := \
	$(EXECUTABLE_PING) \
	$(EXECUTABLE_PONG)

OBJ_PING := \
	local_ping.o \
	util.o
OBJ_PONG := \
	local_pong.o \
	util.o

%.o : %.c
	$(CC) -c $(CFLAGS) $< -o $@

.PHONY: all clean

all: $(ALL_EXECUTABLES)

$(EXECUTABLE_PING): $(OBJ_PING)
	$(CC) $(CFLAGS) $^ -o $@

$(EXECUTABLE_PONG): $(OBJ_PONG)
	$(CC) $(CFLAGS) $^ -o $@

clean: $(ALL_EXECUTABLES) $(OBJ)
	$(RM) $^
