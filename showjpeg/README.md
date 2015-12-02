Fullscreen JPEG viewer
======================

Shows a single JPEG image in fullscreen using dispmanx.

```
./showjpeg test.jpg
```
By default it uses screen 0 and layer -1. To change that, define
the environment variables SCREEN or LAYER.

## building

You will need to install libjpeg before you build the program. On Raspbian

```
sudo apt-get install libjpeg8-dev
```

Then just type 'make' in the showjpeg directory you cloned from github.
If you need a similar tool to display PNG files, have a look at
[Andrews repository](https://github.com/AndrewFromMelbourne/raspidmx/tree/master/pngview).
