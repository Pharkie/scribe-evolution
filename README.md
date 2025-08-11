# Project Scribe

Here's a cool little networked thermal printer for printing whatever you like.

## 🛠️ This fork by Pharkie

This fork includes the following changes to the original repo:

- **ESP32-C3 support** - Changed from the from ESP8266 D1 Mini to ESP32-C3
  Supermini. Now uses hardware serial not software.
- **Configuration centralization** - All settings e.g. wifi password moved to
  `src/config.h`, where they can be kept out of the### Credits and
  Acknowledgments
- **New UI** - You can write on receipt after another more easily, now.
- **mDNS integration** - Device accessible at http://scribe-XXX.local, as well
  as the IP address.
- **MQTT remote printing** - Send messages to other Scribe printers over MQTT,
  supporting both local and remote printers. You'll need your own MQTT broker to
  use this bit (HiveMQ has a free tier).
- **Automatic timezone handling** - timezone handling inc automatic daylight
  savings and improved date handling elsewhere via the ezTime library.
- **Fun buttons** - Added Riddle, Joke, Quote, and Print Test buttons for
  entertainment and printer testing, local or remote.
- **Hardware button support** - Physical GPIO buttons can be wired to trigger
  any web endpoint (riddles, jokes, quotes, etc.) for tactile interaction.
- **Comprehensive logging system** - System logging with multiple output
  destinations (Serial, LittleFS file, MQTT, BetterStack).
- **Enhanced robustness** - new text wrapping algorithm, WiFi reconnection,
  watchdog timer, and device status monitoring.
- **Character mapping resilience** - Better handling of Unicode and special
  characters, so more resilient to character encoding issues.
- **Pin stabilization** - Quick stablisation of the TX pin on boot to stop the
  printer from hanging during initialisation.
- **Web server upgrades** - Favicon and 404 pages included

The below is a mix of the original and my changes.

All credit to UrbanCircles for the original concept, 3D model and original code.
Have fun, makers!

#### Key info

I could not power the Cashino CSN-A4L via USB, whatever cable or supply I used.
It appears you can ONLY power the printer via the 3 pin (2 pins used) "POWER"
connector, and then USB could perhaps be used for serial. I didn't investigate
that, because we use the TTL 5-pin connection instead which, just to be clear,
does not provide power (but it does reference ground).

When you first get the printer, you want to check it works, right? Hold down the
front (only) button as you power on the printer to do a self test. i.e. hold the
button and attach 5V power to the POWER 3 pin (2 pins used) connector (I did
this using a bench power supply and crocodile clips).

### API data sources

The project integrates with several external APIs and local databases for
enhanced functionality:

- **Jokes**: Powered by [icanhazdadjoke.com](https://icanhazdadjoke.com/) - A
  free API providing random dad jokes
- **Quotes**: Powered by [ZenQuotes.io](https://zenquotes.io/) - Free
  inspirational quotes API
- **Quiz**: Powered by [The Trivia API](https://the-trivia-api.com/) - Free
  trivia questions with multiple choice answers
- **Riddles**: Local database with 545+ riddles curated by
  [Nikhil Mohite](https://github.com/nkilm/riddles-api) - stored locally

---

## Build System

This project uses npm for asset building and PlatformIO for firmware
compilation.

### CSS & JavaScript Assets

The web interface uses Tailwind CSS and minified JavaScript:

```bash
npm install                # Install build dependencies
npm run build             # Build CSS + JS (with source maps)
npm run build-prod        # Production build (no source maps)
```

**Note:** The minified files (`data/css/tailwind.css` and `data/js/app.min.js`)
are committed to the repository for users who don't have Node.js installed. If
you modify CSS or JS files, rebuild the assets before committing.

### Enhanced Upload Process

Use the enhanced upload script for automatic asset building:

```bash
pio run --target upload_all   # Builds assets + uploads filesystem + firmware
```

This automatically:

1. 🎨 Builds and minifies CSS/JavaScript
2. 📁 Uploads filesystem to ESP32
3. 💾 Uploads firmware to ESP32

---

## Setup Instructions for PlatformIO

### 1. Configuration Setup

Before building the project, you need to create your configuration file:

1. Copy `src/config.h.example` to `src/config.h`
2. Edit `src/config.h` with your WiFi credentials and settings
3. The `config.h` file is ignored by git to keep your credentials safe

```bash
cp src/config.h.example src/config.h
# Edit src/config.h with your WiFi credentials
```

### 2. Build and Upload

```bash
pio run                    # Build the project
pio run --target upload    # Upload to ESP32-C3
pio device monitor         # Monitor serial output
```

> [!TIP]  
> This is the base configuration of Scribe, to serve one straight forward,
> important use case: it turns the Scribe platform into a simple and reliable
> short message writer (simple by design). It works very well as is.
>
> However, if you wish, you can go beyond this and make this human-machine/
> human-computer interface truly your own.
>
> The hardware is capable, the design is easily adaptable, and the firmware is
> easy to develop/ scale in whichever direction your imagination leads you
> towards! I'm excited to see what you come up with!

Base Scribe: a simple, reliable, open-source system that helps you capture the
meaningful moments of your life. It leverages thermal printing to write a
tangible log of your life's story, daily achievements, thoughts, or memories on
a continuous roll of paper. It's designed to be a quiet companion that nudges
you to live more intentionally.

This project is born from the idea of "bringing receipts" for the life you lead/
towards the life you want to lead. It's about creating a physical artifact of
your journey. It is highly hackable, adaptable and scalable to fit your needs
and wants.

## 📚 Documentation

- **[Code Structure Guide](docs/code-structure.md)** - Documentation of the
  modular codebase architecture, including directory structure, component
  responsibilities, and development guidelines.

## Features of the "out-of-the-box" configuration (v1)

- **Thermal Printing:** Clean, permanent-enough tactile records - no mess, no
  dried ink
- **Standard Rolls:** Very cheap and available paper - e.g. couple of bucks for
  6 rolls
- **Minimalist Web Interface:** Sleek, distraction-free input with just a text
  box and submit button (and some confetti for that extra delight)
- **API Integration:** Integrate Project Scribe with your favourite tools like
  Apple Shortcuts, IFTTT, or your own custom scripts.
- **Offline Operation:** No cloud services required (but cloud capable) -
  everything works locally
- **Organic Design:** Loosely Inspired by Dalí and Gaudí, with flowing curves
  that hint at life's organic nature
- **Low Power:** Runs on a single USB port, no separate power supply needed
  (about 0.5W at idle)
- **Open Source:** Completely free, hackable, and customisable

![pic for profiles](https://github.com/user-attachments/assets/56afd51b-3560-419a-93f4-af315ba2968f)

## BOM

- ESP32-C3 MCU board (other ESP32 boards may work with pin adjustments)
- CSN-A4L thermal printer (other serial thermal printers might work)
- Paper rolls (printer comes preloaded with one, and it'll last ages. You're
  looking for 57.5±0.5mm width and 30mm max diameter)
- 3D printer for the body (you may need some glue to fix the parts together - no
  screws required)
- Wires (for soldering and connecting components + USB wire to power the whole
  thing)
- 5V/ USB power supply capable of higher currents (only used during thermal
  printing)

Links to the 3D printed component files can be found in the
[Assembly](#assembly) section contains.

If you don't have a 3D printer but would like to build this, consider using the
PCBWay affiliate link: https://pcbway.com/g/86cN6v (discount to you + some small
help for the project).

### Affiliate links for the components

If you're so inclined, feel free to use the following affiliate links. They help
out the project :)

> [!NOTE] The components might be slightly different as listings always change
> silently - always check. If you notice any issues, please ping me to update
> the readme.

| Component                         | Amazon US               | Amazon UK               | AliExpress                                |
| --------------------------------- | ----------------------- | ----------------------- | ----------------------------------------- |
| Microcontroller (ESP32-C3)        |                         |                         | -                                         |
| Thermal Printer (CSN-A4L)         | https://amzn.to/4kr5ksq | -                       | https://s.click.aliexpress.com/e/_opjoNrw |
| Paper Rolls, BPA-free (57.5x30mm) | https://amzn.to/4kpOREP | https://amzn.to/44nqGCg | -                                         |

> [!IMPORTANT] Do your own due diligence regarding thermal paper types - the
> thermal paper we handle everyday (e.g. through receipts from the grocery
> store, restaurants, takeaway, taxis, etc.) will contain BPA. When choosing
> your rolls for this, you should definitely go for BPA-free paper just to be on
> the safer side - the links provided are for BPA-free paper. If you can, go a
> step further and look for "phenol-free" paper. Three types that do not contain
> BPA or BPS and are competitively priced contain either ascorbic acid (vitamin
> C), urea-based Pergafast 201, or a technology without developers, Blue4est.

> [!NOTE] Some thermal paper is treated against fading - can last e.g. 35+
> years. If you're planning on using Scribe for archival purposes, consider ink
> fading when picking up the right rolls.

## Pin-out/ wiring during operation

The project uses UART1 to communicate with the printer on the ESP32-C3.

| Printer Pin | ESP32-C3 Pin | Power Supply Pin | Description              |
| ----------- | ------------ | ---------------- | ------------------------ |
| TTL RX      | GPIO20       | -                | MCU Transmit             |
| TTL TX      | GPIO21       | -                | MCU Receive (not needed) |
| TTL GND     | GND          | GND              | Common Ground            |
| Power VH    | -            | 5V               | Printer VIN              |
| Power GND   | GND          | GND              | Printer GND              |

Wires not listed in the table (e.g. TTL NC/ DTR) are unused andcan be removed.
Fewer wires => less clutter which is hugely helpful.

> [!IMPORTANT] Never power the printer directly from/ through the ESP32-C3, you
> may burn your microcontroller.
>
> **Only power the ESP32-C3 via one source** - either via USB during firmware
> flashing, or via the 5V pin during normal operation from the shared power
> supply.

## MQTT Remote Printing

My fork includes MQTT support for networked printing between multiple Scribe
devices:

### Features

- **Multiple printer support** - Configure multiple remote Scribe printers in
  `config.h`
- **Unified interface** - Single web UI supports local direct, local via MQTT,
  and remote printing
- **Secure connection** - Uses TLS encryption (port 8883) for secure MQTT
  communication
- **Cloud MQTT broker** - Ready for e.g. HiveMQ Cloud (free tier) (configurable
  in `config.h`)

### Configuration

1. Set up your MQTT broker credentials in `src/config.h`:

   ```cpp
   static const char *mqttServer = "your-broker.hivemq.cloud";
   static const char *mqttUsername = "your-username";
   static const char *mqttPassword = "your-password";
   ```

2. Configure printer topics:

   ```cpp
   // This device's MQTT configuration
   static const char *localPrinter[2] = {"Your Printer", "scribeprinter/yourname/inbox"};

   // Other printer configurations
   static const char *otherPrinters[][2] = {
       {"Friend's Printer", "scribeprinter/friend/inbox"}
   };
   ```

### Quick Action Buttons

The web interface includes quick action buttons that work with any selected
printer:

- **🧩 Riddle** - Prints a random riddle from the built-in collection
- **😂 Joke** - Fetches and prints a random dad joke from an online API
- **💭 Quote** - Fetches and prints inspirational quotes from ZenQuotes API
- **🧠 Quiz** - Fetches and prints trivia questions with multiple choice answers
- **🔤 Test Print** - Prints a comprehensive character set test for printer
  calibration (positioned last for optimal UX flow)

All quick actions include timestamps and work seamlessly with both local and
remote printing.

### MQTT Message Formats

The MQTT handler supports two message formats for direct MQTT communication:

1. **Simple Text Messages**:

   ```json
   { "message": "Your text to print" }
   ```

2. **Endpoint Actions** (for quick actions via MQTT):
   ```json
   {"endpoint": "/joke"}
   {"endpoint": "/riddle"}
   {"endpoint": "/quote"}
   {"endpoint": "/quiz"}
   {"endpoint": "/test"}
   ```

Both formats are processed through the unified endpoint system with proper
watchdog feeding to prevent system crashes during content generation.

**Note**: The Pipedream HTTP-to-MQTT bridge is specifically for Unbidden Ink
messages and only uses the simple text message format. Quick actions are
triggered directly from the web interface or via direct MQTT clients.

## 🌐 Print from Apple Shortcuts etc

### Pipedream MQTT Bridge (Recommended)

Since e.g. Apple Shortcuts can't send to MQTT directly, and you aren't always at
home with the Scribe on the local network, you can send messages to your Scribe
printer from anywhere via an HTTP-to-MQTT bridge.

**📖 [Pipedream Setup Guide](docs/pipedream-integration.md)**

## Logging System

This fork includes a comprehensive logging system built on the ArduinoLog
library:

### Features

- **Multiple Output Destinations**:

  - Serial console for development debugging
  - LittleFS file storage for persistent logs
  - MQTT topic publishing for remote monitoring
  - BetterStack integration for cloud log aggregation

- **Configurable Log Levels**: Uses ArduinoLog's standard levels:

  - `LOG_LEVEL_SILENT` (0) - No output
  - `LOG_LEVEL_FATAL` (1) - Fatal errors only
  - `LOG_LEVEL_ERROR` (2) - Errors and fatals
  - `LOG_LEVEL_WARNING` (3) - Warnings, errors, and fatals
  - `LOG_LEVEL_NOTICE` (4) - Notice, warnings, errors, and fatals
  - `LOG_LEVEL_TRACE` (5) - Trace, notice, warnings, errors, and fatals
  - `LOG_LEVEL_VERBOSE` (6) - All output including verbose debug

### Configuration

Configure logging in `src/config.h`:

```cpp
// Set your desired log level
static const int logLevel = LOG_LEVEL_NOTICE;

// Enable/disable output destinations
static const bool logToSerial = true;        // Development console
static const bool logToFile = false;         // LittleFS file (/logs/scribe.log)
static const bool logToMQTT = false;         // MQTT topic (scribe/log)
static const bool logToBetterStack = false;  // BetterStack cloud service

// BetterStack configuration (if enabled)
static const char *betterStackToken = "your-token-here";
```

The logging system provides detailed information about system operations, WiFi
connectivity, MQTT messages, printer activity, and API responses, making
debugging and monitoring much easier.

## Microcontroller firmware

### Configuration

All configuration is handled in `src/config.h` after copying from
`src/config.h.example`. This includes:

- WiFi credentials (SSID and password)
- Timezone settings (automatically handles DST with ezTime library)
- mDNS hostname (default: "scribe" for http://scribe.local access)
- MQTT broker settings for remote printing
- Logging system configuration (levels, output destinations)
- Character limits and other preferences
- Professional favicon automatically served to prevent 404 errors

### Development Environment

**Recommended:** VS Code with PlatformIO extension for the best development
experience:

1. Install [VS Code](https://code.visualstudio.com/)
2. Install the [PlatformIO IDE extension](https://platformio.org/platformio-ide)
3. Open the project folder in VS Code
4. PlatformIO will automatically handle dependencies and board configuration

**Dependencies:** The project uses several libraries automatically managed by
PlatformIO:

- **ArduinoLog** - Professional logging framework with multiple output support
- **ArduinoJson** - Robust JSON parsing and generation for API responses
- **ezTime** - Advanced timezone handling with automatic DST transitions
- **PubSubClient** - MQTT client for remote printing capabilities
- **LittleFS** - File system for web assets and log storage

**Alternative:** You can also use the
[Arduino IDE](https://www.arduino.cc/en/software/) with ESP32 board support,
though PlatformIO is recommended for dependency management.

Ensure that everything is working **before** soldering, and squeezing your
components into the 3D printed shell!

> [IMPORTANT!] As mentioned above - do not power the printer through the
> ESP32-C3 and do not power the ESP32-C3 via both the USB and its pins at the
> same time.

## Assembly

In each set, there are 2x 3D Printed components:

- The head unit (in which your MCU + Thermal Printer + wiring slot it)
- The neck/ leg (connects with the head and has a channel to elegantly route/
  feed your power cable through)

The printed components can be found on either the
[Maker World](https://makerworld.com/en/models/1577165-project-scribe#profileId-1670812)
page, or the
[Printables](https://www.printables.com/model/1346462-project-scribe/files) page

**Printing considerations**

- The head has fillets, so you may need supports
- Smaller line heights will produce better results
- The neck/ leg can be printed without any supports upright
- The components may vary in size slightly, so will the tolerances/ clearances -
  you may need to us glue to put the pieces together in case they're lose, or
  sandpaper in case they're too tight

**Assembly considerations**

- Make sure you route the wire through the neck/ leg of Scribe before you crimp
  the connectors
- Important: make sure each connection and wire is well isolated before you cram
  all the wiring into the head unit! You really don't want a short circuit
- Always to a test run before final assembly
- Do not glue the electrical components together, in case you need to service
  this later (you shouldn't need to glue them together)

## User guide for standard configuration (v1)

1. **Power On:** Connect the device to a beefy (2.4A+) 5V USB power source. Wait
   a few moments for it to boot, connect to WiFi, and print its startup receipt.
2. **Access the Interface:** Open http://scribe.local in your browser (or use
   the IP address from the startup receipt if mDNS doesn't work).
3. **Start Scribing!**
4. **Look back at your story**
5. **Improve and scribe some more**

**Message format** The as-is firmware prints messages in the right orientation
for the roll of paper to naturally wind downwards, with word wrap. The first
line is the header, reminiscent of a calendar - date on black background. The
following lines are the message itself.

**Scribing through a web browser**

- The ESP32-C3 creates a local web server and the as-is configuration includes a
  minimalist, light web app
- Open a web browser on any device on the same network and navigate to
  `http://scribe.local` (or `http://<IP_ADDRESS>`). Type your entry (up to 200
  characters) and press Enter or click the "Send" button.
- This limitation is not the limitation of the printer/ hardware/ system, I just
  like it to keep the messages concise - you can change it in firmware (just
  like everything else around here)

**Scribing through the API**

- You can also send entries directly from a browser or script. For example:
  `http://scribe.local/submit?message=Went%20for%20a%20hike`
- This is particularly useful when running automations - it works straight out
  of the box
- Different to the web app, when using the API there is no character limit out
  of the box. In addition, you can also backdate your entries, by adding the
  `date` parameter:
  `http://scribe.local/submit?message=Finished%20the%20book&date=2025-07-04`

## Webhook Integration with n8n

For users who want to integrate Project Scribe with external services like Apple
Shortcuts, IFTTT, or other web-based automation tools, **n8n** provides an
excellent bridge between HTTP webhooks and MQTT.

### What is n8n?

**n8n** is an open-source workflow automation tool that you can use to connect
different services and APIs. In this case n8n can act as a translator,
converting incoming HTTP POST requests (webhooks) into MQTT messages that your
Scribe printer will understand.

### Why Use n8n?

- **Universal Integration**: Works with Apple Shortcuts, IFTTT, Zapier, and any
  service that can send HTTP POST requests
- **No Local Networking Required**: External services can reach your printer
  through MQTT without needing access to your local network, poking holes in the
  firewall, port forwarding or ngrok.
- **Secure**: Uses encrypted MQTT connections (TLS) rather than exposing your
  local printer to the internet
- **Flexible**: Can process, format, or validate messages before sending them to
  your printer

### Setting up n8n

1. **Host n8n**: You'll need to host n8n somewhere accessible from the internet.
   Popular options include:

   - **Fly.io** (recommended for beginners) - Free tier available
   - **Railway** - Simple deployment with free tier
   - **DigitalOcean** - More control, requires basic server management
   - **Self-hosted** - On your own VPS or cloud server

2. **Create a Webhook to MQTT Workflow**:

   ```
   HTTP Webhook → (Optional: Data Processing) → MQTT Node
   ```

3. **Configure the Webhook Node**:

   - Set up a webhook endpoint (e.g., `https://your-n8n.fly.dev/webhook/scribe`)
   - Accept POST requests with JSON payload

4. **Configure the MQTT Node**:
   - **Broker**: Your HiveMQ Cloud endpoint (from `config.h`)
   - **Topic**: Your printer's MQTT topic (e.g., `scribe/YourPrinter/inbox`)
   - **Message**: Extract message from webhook payload

### Example Apple Shortcuts Integration

With n8n set up, you can create an Apple Shortcut that:

1. Prompts for text input
2. Sends POST request to your n8n webhook:
   `https://your-n8n.fly.dev/webhook/scribe`
3. n8n forwards the message to your printer via MQTT

**Shortcut POST Body Example**:

```json
{
  "message": "Your message text here",
  "printer": "scribe/YourPrinter/inbox"
}
```

This setup allows you to securely print to any Scribe printer from anywhere in
the world using just your phone.

## Beyond the as-is: Ideas to Extend or Replace the As-Is Functionality

The combination of a WiFi-enabled MCU, a Thermal Printer and an API-ready web
server makes Scribe a powerful platform for all sorts of creative projects.

You can easily adapt the existing code to create a completely new experience.
Here are a few ideas to get you started:

- **Daily Briefing Printer:** Modify the code to fetch data from public APIs
  every morning. It could print:
  - Your first few calendar events for the day.
  - The local weather forecast.
  - A curated news headline from an RSS feed.
  - The word or quote of the day.
- **Task & Issue Tracker:** Connect it to your productivity tools (like Todoist,
  Jira, or GitHub) via their APIs or webhooks.
  - Print new tasks or tickets as they are assigned to you.
  - Print your most important tasks for the day each morning.
- **Kitchen Companion:** Place it in your kitchen to print:
  - Shopping lists sent from a family messaging app or a shared note.
  - A recipe of the day.
  - Measurement conversion charts on demand.
- **Tiny Message Receiver:** Create a unique, private messaging system. Family
  members could send short messages to the printer from anywhere, creating a
  physical message board.
- **Daily Dose of Fun:** Make it a source of daily delight by having it print:
  - A random joke or comic strip (like XKCD).
  - A "shower thought" from a Reddit subreddit.
  - A custom fortune cookie message.
- **Photo Booth Printer:** Extend the functionality to accept an image URL via
  the API, dither the image in the firmware, and print low-resolution, stylised
  versions of your photos.

## Troubleshooting

In my testing and usage, I found this setup to be extremely reliable (after all,
these printers are used in commercial settings). If the device is not printing
as expected, this may be because of several reasons, e.g.:

- incorrect wiring/ a short
- paper not inserted correctly
- current on offer is not high enough

Thermal Printer Manual, in case you need to look into things further:
https://www.manualslib.com/manual/3035820/Cashino-Csn-A4l.html

## Credits and Acknowledgments

### Riddles Collection

The riddles feature uses a collection of riddles curated by **Nikhil Mohite**
from the [riddles-api](https://github.com/nkilm/riddles-api) project. This
collection is provided under the MIT License.

- **Original Repository:** https://github.com/nkilm/riddles-api
- **Author:** Nikhil Mohite
- **License:** MIT License

We thank Nikhil for making this wonderful collection of riddles available to the
open source community.

### Original Project

All credit to **UrbanCircles** for the original Project Scribe concept, 3D
model, and initial codebase that made this ESP32-C3 thermal printer possible.

## Disclaimer

I've done my best to document everything accurately - however, there might be
mistakes. If you see them, or opportunities to improve, please open an issue.  
This is an open-source project given for free, with no warranties or guarantees.
It assumes a level of proficiency with electronics, assemblies, engineering,
etc. Do your own due diligence - it's your responsibility. Stay safe. Stay
productive. Work with what you have. Make the world a better place.

## License

This project is licensed under the **Creative Commons
Attribution-NonCommercial-ShareAlike 4.0 International License**.

### What this means:

✅ **You are free to:**

- **Share** — copy and redistribute the material in any medium or format
- **Adapt** — remix, transform, and build upon the material

⚠️ **Under the following terms:**

- **Attribution** — You must give appropriate credit to Adam Knowles, provide a
  link to the license, and indicate if changes were made
- **NonCommercial** — You may not use the material for commercial purposes
  without explicit written permission
- **ShareAlike** — If you remix, transform, or build upon the material, you must
  distribute your contributions under the same license

### Commercial Use

**Commercial use is prohibited without a licensing agreement.** If you wish to
use this project commercially (including but not limited to selling products
based on this design, using it in commercial environments, or incorporating it
into commercial software), please contact the authors.

### Full License

You can view the full license text at:
https://creativecommons.org/licenses/by-nc-sa/4.0/

### Copyright (extended code)

© 2025 Adam Knowles. All rights reserved.

**Original Project:** Based on the original Project Scribe by UrbanCircles, used
with permission and adapted under the same Creative Commons license terms.
