A tiny VNC Server for info-beamer
=================================

This VNC server can be used to peek into a running
info-beamer instance. Start the server on the raspberry pi
and connect to it using any VNC viewer.

To compile, first install libvncserver-dev, then type make.

```
$ apt-get install libvncserver-dev
$ make
```

This VNC Server is based on code by Peter Hanzel 
https://github.com/hanzelpeter/dispmanx_vnc
