CFLAGS  += -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM -std=gnu99 -fgnu89-inline 
CFLAGS  += -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux
CFLAGS  += -O3 -Wno-psabi -s

all: screenshot screenshot-fb

screenshot: screenshot.c
	$(CC) $(CFLAGS) $< -L/opt/vc/lib/ -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -ljpeg -o $@

screenshot-fb: screenshot-fb.c
	$(CC) $(CFLAGS) $< -ljpeg -o $@

install: screenshot
	install -D -o root -g root -m 755 $< /usr/local/bin/$<

clean:
	rm -f screenshot screenshot-fb

.PHONY: clean
