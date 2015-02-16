A visual watchdog for the Raspberry PI
======================================

Visual presentations on the PI could get stuck for various
reasons. It might be useful to detect and respond to that.

This tool takes snapshots of the current output displayed
on the screen. It uses a configurable interval.

Each time a snapshot was taken it calculates a simple 
checksum and compares it to the previous checksum. If the
checksum was unchanged (which probably means that the output
on the screen hasn't changed) it will increment a "stuck"
counter. If that counter passes a configurable threshold
the program exists with exit status 0 and you can take
actions.

Configuration
-------------

Settings are provided using environment variables:

`WATCHDOG_INTERVAL` - seconds between taking snapshots

`WATCHDOG_COUNT` - number of times a checksum can be equal
to the previous value without triggering.

Usage:
------

Build the tool by running `make`. Then start it:

`WATCHDOG_INTERVAL=1 WATCHDOG_COUNT=3 ./visual-watchdog && echo "it's stuck"`

The tool will only exit with error code 0 if it detects that the
output didn't change.
