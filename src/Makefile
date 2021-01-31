CFLAGS=-Wall -g -DPHANT5

objs = main.o \
	handlers.o \
	ui.o

phantcli: $(objs)
	gcc $(CFLAGS) -o $@ $^ -lncurses

$(objs): %.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(objs)

$(objs): packet.h phantcli.h
