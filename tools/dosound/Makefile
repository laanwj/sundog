CFLAGS=-O2 -Wall
LDFLAGS=
OBJS=dosound.o emu2149.o
LINK=$(CC)

dosound: ${OBJS}
	$(LINK) $^ $(LDFLAGS) -o $@
    
clean:
	rm -f ${OBJS} dosound
