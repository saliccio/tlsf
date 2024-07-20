# Makefile for TLSF library and test executable

# Compiler and compiler flags
CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGET_LIB = libtlsf.a
TARGET_TEST = test

SRCS_LIB = tlsf.c
SRCS_TEST = test.c

OBJS_LIB = $(SRCS_LIB:.c=.o)
OBJS_TEST = $(SRCS_TEST:.c=.o)

all: $(TARGET_LIB) $(TARGET_TEST)

$(TARGET_LIB): $(OBJS_LIB)
	ar rcs $@ $^

$(TARGET_TEST): $(OBJS_TEST) $(TARGET_LIB)
	$(CC) $(CFLAGS) -o $@ $^ -L. -ltlsf

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS_LIB) $(OBJS_TEST) $(TARGET_LIB) $(TARGET_TEST)

.PHONY: all clean