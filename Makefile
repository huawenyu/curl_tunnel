TARGET = curltunnel
OBJS = libcurl_tunnel.o
LIBS = -lcurl

all: $(TARGET)

$(TARGET): $(OBJS)
    $(CC) -o $(TARGET) $(LDFLAGS) $(LIBS) $(OBJS)

libcurl_tunnel.o: libcurl_tunnel.c

clean:
    rm -f $(TARGET) $(OBJS)

