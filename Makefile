CC = gcc
CFLAGS = -g -O0 -Wall -Wextra

SRCS = demux.c main.c
OBJS = $(SRCS:.c=.o)
TARGET = executable

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(TARGET)	