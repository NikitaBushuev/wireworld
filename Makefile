CFLAGS := `sdl2-config --cflags`

LDLIBS := `sdl2-config --static-libs`

OBJS := wireworld.o

.PHONY: all clean

all: wireworld

clean:
	$(RM) $(OBJS)

wireworld: $(OBJS)
	$(CC) -o $@ $(CFLAGS) $^ $(LDLIBS)