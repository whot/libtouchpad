This is a driver for multi-touch capable clickpads.

First instance is mainly the X driver, future versions will focus on wayland
etc. as well.

This driver requires:
* the Linux evdev kernel API
* a multitouch-capable clickpad

This driver provides:
* a "swapable" backend (making it usable for X, wayland compositors, etc.)
* tools for console debugging

Features (current):
* it builds
* tap-to-click
* two-finger scrolling
* software button emulation

Features (eventually):
* edge scrolling
* clickfingers

TODO
====
* drop callbacks, switch to list of events instead
* make the test suite use different devices automatically
** requires all coords be given in percent of the touchpad
* optimise the loops through the touch points
* single-touch devices can map ABS_X to a single touchpoint and use this driver too
* we need to support more than one button set, the T440s use multiple button
  layouts
