Raspberry PI screen grabber
===========================

This tools captures to current output of the Raspberry PI and
writes it as JPEG to stdout.

```
$ ./screenshot > snapshot.jpg
```

By default jpeg writing uses a quality level of 75. This is
probably a good compromise between quality and file size.
To change the quality level just provide an parameter:

```
$ ./screenshot 90 > snapshot.jpg
```

## Building

You will need to install libjpeg before you build the program. On Raspbian

sudo apt-get install libjpeg8-dev

Then just type 'make' in the screenshot directory you cloned from github.
