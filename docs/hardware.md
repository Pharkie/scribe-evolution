# Hardware Guide

This document covers the hardware requirements, wiring, assembly, and printer-specific information for the Scribe ESP32-C3 Thermal Printer project.

## Bill of Materials (BOM)

### Required Components

- **ESP32-C3 MCU board** - Main microcontroller (other ESP32 boards may work with pin adjustments)
- **CSN-A4L thermal printer** - Thermal printing unit (other serial thermal printers might work)
- **Paper rolls** - 57.5±0.5mm width and 30mm max diameter (printer comes preloaded with one)
- **3D printed enclosure** - 2-part design (head unit and neck/leg)
- **Wires** - For connections between components
- **USB cable** - For power and programming
- **5V USB power supply** - Must be capable of higher currents (2.4A+ recommended, only needed during thermal printing)

### Component Links

#### 3D Printed Components

The printed components are available on:

- [Maker World](https://makerworld.com/en/models/1577165-project-scribe#profileId-1670812)
- [Printables](https://www.printables.com/model/1346462-project-scribe/files)

If you don't have a 3D printer, consider using the PCBWay affiliate link: https://pcbway.com/g/86cN6v

#### Affiliate Component Links

> [!NOTE] The components might be slightly different as listings change. Always verify specifications before purchasing.

| Component                         | Amazon US               | Amazon UK               | AliExpress                                |
| --------------------------------- | ----------------------- | ----------------------- | ----------------------------------------- |
| Microcontroller (ESP32-C3)        | -                       | https://amzn.eu/d/ebTV8DC | https://www.aliexpress.com/item/1005007499955013.html?spm=a2g0o.order_list.order_list_main.15.329c1802O41vw5                                         |
| Thermal Printer (CSN-A4L)         | https://amzn.to/4kr5ksq | -                       | https://www.aliexpress.com/item/32994269074.html?spm=a2g0o.order_list.order_list_main.36.76a91802Kn45St |
| Paper Rolls, BPA-free (57.5x30mm) | https://amzn.to/4kpOREP | https://amzn.to/44nqGCg | -                                         |

For the AliExpress printer, order the vairant "Color: White TU 5-9V" (i.e. TTL not RS232 and 5-9v not 12v
)
### Thermal Paper Safety

> [!IMPORTANT] **BPA-Free Paper Recommended**
>
> Standard thermal paper (like grocery receipts) contains BPA. For this project, choose BPA-free paper for health safety. The linked products are BPA-free.
>
> For maximum safety, look for "phenol-free" paper. Safe alternatives contain:
>
> - Ascorbic acid (vitamin C)
> - Urea-based Pergafast 201
> - Blue4est technology (no developers)

> [!NOTE] **Archival Considerations**
>
> Some thermal paper is treated against fading and can last 35+ years. If using Scribe for archival purposes, consider ink longevity when selecting paper rolls.

## Pin-out and Wiring

### ESP32-C3 to CSN-A4L Printer Connection

The project uses UART1 to communicate with the printer:

| Printer Pin | ESP32-C3 Pin | Power Supply Pin | Description              |
| ----------- | ------------ | ---------------- | ------------------------ |
| TTL RX      | Configurable in software, defaults to GPIO21 | -                | MCU Transmit             |
| TTL GND     | GND          | GND              | Common Ground            |
| Power VH    | -            | 5V               | Printer VIN              |
| Power GND   | GND          | GND              | Printer GND              |

**Unused connections:**

- TTL TX (the printer doesn't reply to the MCU at all, traffic is one-way)
- TTL NC (Not Connected)
- TTL DTR (Data Terminal Ready)

These wires can be removed to reduce clutter during assembly.

### Power Requirements

> [!IMPORTANT] **Power Safety**
>
> - **Never power the printer directly from the ESP32-C3** - you may damage the microcontroller
> - **Only power the ESP32-C3 via one source** - either USB (during programming) OR via the 5V pin (during operation)
> - Use a dedicated 5V power supply capable of 2.4A+ for the shared power rail

### Wiring Diagram

```
5V Power Supply
├── VCC → Printer Power VH
├── GND → Common Ground Rail
│
ESP32-C3
├── GPIOXX (pick one) → Printer TTL RX
├── GND → Common Ground Rail
├── 5V ← Power Supply (during operation)
└── USB ← Programming cable (programming only)
```

## CSN-A4L Printer Specific Information

### Power Requirements

The CSN-A4L thermal printer cannot be powered via USB - it requires the dedicated POWER connector:

- **Power connector**: 3-pin (2 pins used)
- **Voltage**: 5V
- **Current**: High current during printing (reason for 2.4A+ power supply requirement)

### Testing the Printer

Before assembly, test your printer:

1. Hold down the front button
2. Apply 5V power to the POWER connector (2 pins)
3. The printer should perform a self-test and print a test pattern
4. Use a bench power supply with crocodile clips for initial testing

### Serial Communication

- Uses TTL serial communication (5V logic levels)
- Baud rate: 115200 (configured in firmware)
- Only TX from ESP32 to printer RX is required for operation
- RX from printer is optional (not used in current firmware)

## 3D Printing Guidelines

### Print Settings

**Head Unit:**

- Requires supports due to fillets and overhangs
- Use smaller layer heights (0.1-0.2mm) for better surface finish
- Print orientation: electronics cavity facing up

**Neck/Leg:**

- Can be printed upright without supports
- Standard layer heights acceptable (0.2-0.3mm)
- Ensure cable channel is properly formed

### Post-Processing

- Test fit components before final assembly
- Tolerances may vary between printers:
  - **Too loose**: Use CA glue or epoxy for permanent bonding
  - **Too tight**: Sand contact surfaces carefully
- Do not glue electrical components - leave serviceable

## Assembly Instructions

### Pre-Assembly Checklist

1. **Test all electrical components** before enclosing them
2. **Route cables first** - thread power cable through neck/leg before making connections
3. **Prepare connections** - ensure all solder joints are clean and well-insulated
4. **Functional test** - verify printing works before final assembly

### Assembly Process

1. **Route Power Cable:**
   - Thread USB power cable through neck/leg channel
   - Leave sufficient length for internal connections

2. **Prepare Electronics:**
   - Solder connections according to wiring diagram
   - Test continuity and isolation of all connections
   - Heat shrink or tape all exposed connections

3. **Test Assembly:**
   - Connect power and test basic functionality
   - Print a test page to verify operation
   - Check for any shorts or connection issues

4. **Final Assembly:**
   - Carefully place electronics in head unit
   - Ensure no wires are pinched or stressed
   - Secure components without permanent adhesive (in case service is needed)
   - Fit neck/leg to head unit

### Assembly Tips

- **Cable Management**: Route wires neatly to avoid interference with paper feed
- **Isolation**: Double-check all electrical connections are properly insulated
- **Access**: Leave some access to connections for future troubleshooting
- **Testing**: Always perform a final test print after assembly

### Tools Required

- Soldering iron and solder
- Wire strippers
- Heat shrink tubing or electrical tape
- Multimeter (for continuity testing)
- Small files or sandpaper (for fit adjustments)
- CA glue or epoxy (if permanent bonding is needed)

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

- Verify GPIO pin from MCU to printer TTL RX connection
- Check baud rate configuration (115200)
- Test with serial monitor for communication

**Garbled output:**

- Check ground connections
- Verify 5V/3.3V logic level compatibility
- Inspect for loose connections

### Mechanical Issues

**Paper feeding problems:**

- Check for obstructions in paper path
- Verify paper roll orientation
- Ensure proper paper tension

**Assembly fit issues:**

- Check 3D print quality and dimensional accuracy
- File or sand contact surfaces for better fit
- Consider reprinting problematic parts
