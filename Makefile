TARGET = curltunnel

CFLAGS=#-std=c99
CFLAGS+=-MMD   # dependency generation magic
CFLAGS+=-W -Wall
LIBS=-lm -lcurl
FLAGS=

SRCS=curl_tunnel.c
OBJS=$(SRCS:.c=.o)
DEPS=$(OBJS:.o=.d)

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(TARGET) $(CFLAGS) $(FLAGS) $(OBJS) $(LIBS)

-include $(DEPS)  # this makes magic happen

clean:
	-rm -f $(TARGET)
	-rm -f $(DEPS) $(OBJS)
