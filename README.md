backlightd
==========

this program lets you control the brightness of the screen backlight, and the
keyboard backlight, on a macbook air.

usage
-----

press f1 and f2 to control the screen's brightness, f5 and f6 to control the
keyboard's brightness.
hold fn down if `/sys/module/hid_apple/parameters/fnmode` is set to `2`.

this program runs as a daemon, which always responds to key presses.

installation
------------

build and install this as a debian package:

```
dpkg-buildpackage -uc -us
sudo gdebi ../backlightd_0-2_all.deb
```
