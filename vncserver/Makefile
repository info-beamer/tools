CFLAGS  += -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM
CFLAGS  += -I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux
CFLAGS  += -O3 -Wno-psabi 

LDFLAGS +=  -L/opt/vc/lib/ -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lvncserver

vncserver: vncserver.c
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@

clean:
	rm -f vncserver

.PHONY: clean
