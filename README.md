# Charger

This repository contains firmware for a simple 2-cell battery charger built
around an ESP32 (Adafruit QT Py ESP32-S3).  The project was created as an
experimental platform for evaluating the feasibility of rejuvenating 18650 and
21700 lithium‑ion cells, particularly those harvested from consumer power tools
(e.g. Milwaukee packs).  In those packs the manufacturer does not appear to
provide active voltage balancing, so a single cell only a few millivolts outside
its expected range can render the entire pack unusable or a pack of cells unusable cutting the total packs voltage and current.  This firmware drives
two relays to select which cell is being charged and the ESP32 monitors the
cell voltages via calibrated ADC inputs.

## Features

* Relay control for two cells in series; only one relay closed at a time.
* Voltage sensing with calibrated ADC and divider ratio.
* Charge cycle: stop at 4.2 V, rest 5 s, resume if voltage drops below 4.15 V.
* Failure-safe relays are active-low (energized = open).
* TODO lists in source for hardware over‑voltage/current/temperature protection
* TODO: make thresholds and other parameters user–configurable in software.

The development effort included tests on deeply discharged cells with some
limited success; results are anecdotal and further research is required before
recommending any practice for battery refurbishment.
## Hardware

The circuit consists of two battery cells in series, two SPDT relays, and the
ESP32 board.  Each cell voltage is scaled by a resistor divider and read on an
ADC pin.

> **Warning:** This code is provided for reference only.  Working with
> batteries and high currents can be dangerous; design appropriate hardware
> protections before using in a real product.

## Building

The project uses PlatformIO.  To build:

```sh
cd Charger
platformio run
```

## Uploading

```sh
platformio run --target upload
```

## Testing

The `test/` directory is reserved for PlatformIO unit tests; none are
implemented yet.

## License

Add your license information here.

---

See `src/main.cpp` for the current implementation and TODO items.