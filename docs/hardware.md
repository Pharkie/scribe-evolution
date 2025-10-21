# Hardware Guide

This document covers the hardware requirements, wiring, and assembly for the Scribe Evolution Thermal Printer project.

## Supported Microcontrollers

Scribe Evolution supports multiple ESP32 variants:

- **ESP32-C3** - Main supported board, compact and cost-effective (4MB flash)
- **ESP32-S3 Mini** - Enhanced variant with more flash (8MB), OTA support capability, and more processing power
- **Lolin32 Lite** - Original ESP32 board (4MB flash)

### Basics

- **ESP32 MCU board** - Choose from supported boards above
- **CSN-A4L thermal printer** - Thermal printing unit or compatible - similar serial thermal printers might work
- **Paper rolls** - 57.5±0.5mm width and 30mm max diameter

### Plus

- **Wires** - For connections between components
- **USB cable** - For power and programming
- **5V USB power supply** - Must be capable of higher currents (2.4A+ recommended, only needed during thermal printing)
- **3D printed enclosure** - 2-part design, head unit plus base.

### Components

#### 3D Printed Enclosure

Until I can release my own design with buttons, you can use the 3D printed components from the original Project Scribe:

- [Maker World](https://makerworld.com/en/models/1577165-project-scribe#profileId-1670812)
- [Printables](https://www.printables.com/model/1346462-project-scribe/files)

If you don't have a 3D printer, please use the original PCBWay affiliate link: https://pcbway.com/g/86cN6v

#### Electronic Components

The components might be slightly different as listings change. It's difficult to provide links for products available in different markets/locations. Check specs before purchase.

| Component                         | Amazon US               | Amazon UK                 | AliExpress                 |
| --------------------------------- | ----------------------- | ------------------------- | -------------------------- |
| Microcontroller (ESP32-C3)        | -                       | https://amzn.eu/d/ebTV8DC | https://is.gd/G1Ymlw       |
| Microcontroller (ESP32-S3 Mini)   | -                       | -                         | Search "ESP32-S3 Mini 8MB" |
| Thermal Printer (CSN-A4L)         | https://amzn.to/4kr5ksq | -                         | https://is.gd/ZZoRLk       |
| Paper Rolls, BPA-free (57.5x30mm) | https://amzn.to/4kpOREP | https://amzn.to/44nqGCg   | -                          |

For EU e.g. Germany, this AliExpress link may work better for the printer:
https://is.gd/kyWcfD

For the AliExpress printer, order the vairant "Color: White TU 5-9V" (i.e. TTL not RS232 and 5-9v not 12v)

### Thermal Paper

#### BPA-free paper recommended

Standard thermal paper (like grocery receipts) contains BPA. For this project, choose BPA‑free paper for health safety.

Look for "phenol‑free" paper. Safe alternatives contain:

- Ascorbic acid (vitamin C)
- Urea-based Pergafast 201
- Blue4est

#### Archival considerations

Some thermal paper is treated against fading and can last 35+ years. If using Scribe Evolution for archival purposes, consider ink longevity when selecting paper rolls.

## Pin-out and Wiring

### ESP32 to CSN-A4L Printer Connection

The project uses ESP32 UART for bidirectional communication with the printer:

| Printer Pin | ESP32 Pin                                          | Power Supply Pin | Description                    |
| ----------- | -------------------------------------------------- | ---------------- | ------------------------------ |
| TTL RX      | Configurable in software (board-specific defaults) | -                | MCU Transmit to printer        |
| TTL TX      | Configurable in software (board-specific defaults) | -                | Printer status/feedback to MCU |
| TTL GND     | GND                                                | GND              | Common Ground                  |
| Power VH    | -                                                  | 5V               | Printer VIN                    |
| Power GND   | GND                                                | GND              | Printer GND                    |

**Note:** Default GPIO pins vary by board type - see board configuration files for your specific ESP32 variant. The printer supports bidirectional communication for status queries and realtime feedback.

**Optional printer connections:**

- TTL NC = Not Connected should be.. not connected.
- TTL DTR (Data Terminal Ready) - optional flow control

Unused wires can be removed from the connector to reduce clutter and confusion.

### Power Requirements

> [!IMPORTANT]
> Power safety
>
> - Never power the printer directly from the ESP32 — you may damage the microcontroller.
> - Power the ESP32 from only one source at a time: either USB (during programming) OR the 5V pin (during operation).
> - Use a dedicated 5V power supply capable of 2.4A+ for the shared power rail.

### Wiring Diagram

```
5V Power Supply
├── VCC → Printer Power VH
├── GND → Common Ground Rail
│
ESP32 (C3/S3/Lolin32)
├── GND → Common Ground Rail
├── 5V ← Power Supply
├── GPIO21 (default; configurable) → Printer TTL RX
└── USB ← cable to PC when programming
│
Thermal Printer (CSN-A4L)
├── Power VH ← 5V Power Supply VCC
├── Power GND → Common Ground Rail
├── TTL RX ← ESP32 TX (GPIO21 default; configurable)
└── TTL GND → Common Ground Rail
```

## CSN-A4L Printer

### Power

The CSN-A4L thermal printer cannot be powered via typical USB ports on e.g. your PC because they don't provide enough power. It requires dedicated power.

- **Power connector**: 3-pin (2 pins used)
- **Voltage**: 5V
- **Current**: High current during printing is reason for 2.4A+ power supply requirement.

### Testing

To test your printer:

1. Hold down the front button
2. Apply 5V power to the POWER connector (2 pins)
3. The printer should perform a self-test and print a test pattern
4. You could use a bench power supply with crocodile clips for initial testing

### Printer manual

https://www.manualslib.com/manual/3035820/Cashino-Csn-A4l.html

## Assembly Instructions

Verify printing works before assembly.

### Assembly Process

1. **Route Power Cable**
   - Thread USB power cable through base channel
   - Leave sufficient length for internal connections

2. **Prepare Electronics**
   - Solder connections according to wiring diagram
   - Test continuity and isolation of all connections
   - Heat shrink or tape all exposed connections

3. **Test**
   - Connect power and test basic functionality
   - Print a test page to verify operation
   - Check for any shorts or connection issues

4. **Final Assembly**
   - Carefully place electronics in head unit
   - Ensure no wires are pinched or stressed
   - Secure components without permanent adhesive (in case service is needed)
   - Fit base to head unit and glue it together

### Tools Required

- Soldering iron and solder
- Wire strippers
- Heat shrink tubing or electrical tape
- Multimeter (for continuity testing)
- Small files or sandpaper (for fit adjustments)
- CA glue or epoxy

## Troubleshooting Hardware Issues

### Power Issues

**Printer not powering on:**

- Check 5V supply capacity (needs 2.4A+)
- Verify power connector wiring
- Test power supply voltage under load

**ESP32 not responding:**

- Ensure only one power source is connected
- Check USB cable and connection
- Try different USB port or power supply

### Communication Issues

**No printing output:**

- Check connection from ESP32 GPIO to printer TTL RX
- Test with serial monitor

**Garbled print output:**

- Indicates floating voltage/signal on the data line from ESP32
- Check ground connections
- Inspect for loose connections

### Mechanical Issues

**Paper feeding problems:**

- Check for obstructions in paper path
- Verify paper roll is the right way around

**Assembly fit issues:**

- Check 3D print quality and dimensional accuracy
- File or sand contact surfaces for better fit
- Consider reprinting problematic parts
