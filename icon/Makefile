wifi: wifi.c icon.c
	gcc -std=gnu99 -DSTANDALONE -D__STDC_CONSTANT_MACROS -D__STDC_LIMIT_MACROS -DTARGET_POSIX -D_LINUX -D_REENTRANT -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -DHAVE_LIBOPENMAX=2 \
		-DOMX -DOMX_SKIP64BIT -pipe -DUSE_EXTERNAL_OMX -DHAVE_LIBBCM_HOST -DUSE_EXTERNAL_LIBBCM_HOST -DUSE_VCHIQ_ARM \
		-Wno-psabi -Wall  -Wno-deprecated-declarations \
		-I/opt/vc/include/ -I/opt/vc/include/interface/vcos/pthreads -I/opt/vc/include/interface/vmcs_host/linux \
		-L/opt/vc/lib/ -lbcm_host -lvcos -lvchiq_arm -lpthread -lrt -lm \
		wifi.c -o wifi

clean:
	rm -f wifi

.PHONY: clean
