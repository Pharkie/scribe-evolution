<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="docs/assets/ScribeLogoMain-white.svg">
    <source media="(prefers-color-scheme: light)" srcset="docs/assets/ScribeLogoMain-black.svg">
    <img alt="Scribe Evolution Logo" src="docs/assets/ScribeLogoMain-black.svg" width="80%" />
  </picture>
</p>

<br>
<br>

<p align="center">
  <table>
    <tr>
      <td width="50%">
        <img src="docs/assets/Scribe-evolution-demo.gif" alt="Scribe Evolution Demo" width="100%" />
      </td>
      <td width="50%">
        <img src="docs/assets/Scribe-original-hardware.jpeg" alt="Scribe Original Hardware" width="100%" />
      </td>
    </tr>
  </table>
</p>

**Print Magic. Share Securely. Poke Friends.**

Scribe Evolution is a delightful, small thermal printer you can use from your phone or laptop. Print quick notes, to‑dos, recipes, riddles, quotes, or little surprises. Keep it on a desk, the kitchen counter, or anywhere you want instant hard copy.

### Reasons to love it

- **Instant printing**: Tear‑and‑go notes in seconds — no ink, no fuss, on supercheap paper.
- **Use any device**: Use the friendly web app at `http://scribe‑[name].local`
- **Private by default**: Runs securely on your network.
- **Remote printing (optional)**: Print from anywhere — set up remote printing ([learn more](docs/mqtt-integration.md))
- **Semi-scheduled surprises**: “Unbidden Ink” (untested) posts fun content throughout your day
- **Share between friends or rooms**: Send any message between Scribe Evolution devices

## Quick Start

### 🚀 **Ready to Go?**

**👉 [Quick Start Guide - Flash the Pre-built Firmware →](docs/quick-start.md)**

Get your Scribe printer running in minutes with pre-built firmware. No coding changes needed.

---

### Hardware setup

- ESP32‑C3 Supermini
- CSN‑A4L thermal printer (or compatible) and paper
- 5V power supply (2.4A+ recommended, important)

To setup your hardware, check the [Hardware Guide](docs/hardware.md).

### Optional extras

The firmware also supports 4 x physical buttons and an addressable LED strip, but you can add these later. 

### Learn more

- **Quick Start**: [Flash pre‑built firmware](docs/quick-start.md)
- **Troubleshooting**: [Common issues and fixes](docs/troubleshooting.md)
- **Remote printing**: [MQTT integration](docs/mqtt-integration.md) and [Apple Shortcuts](docs/apple-shortcuts.md)
- **Docs index**: [See all guides](docs/build-instructions.md)

## Acknowledgments

### Original Project

Credit to **UrbanCircles** for the original Project Scribe concept, 3D
model, and initial codebase that inspired Scribe Evolution.

### Riddles Collection

The riddles feature uses a collection of riddles curated by **Nikhil Mohite**
from the [riddles-api](https://github.com/nkilm/riddles-api) project. This
collection is provided under the MIT License.

- **Original Repository:** https://github.com/nkilm/riddles-api
- **Author:** Nikhil Mohite
- **License:** MIT License

I thank Nikhil for making this wonderful collection of riddles available to the
open source community.

### UI Icons

The user interface icons throughout the application are sourced from high-quality
open source icon libraries:

**Heroicons** - Modern SVG icons crafted by the makers of Tailwind CSS

- **Repository:** https://github.com/tailwindlabs/heroicons
- **Authors:** Steve Schoger and Adam Wathan (Refactoring UI Inc.)
- **License:** MIT License
- **Copyright:** © 2020 Refactoring UI Inc.

**Sidekick Icons** - Carefully designed outline icons for modern applications

- **Repository:** https://github.com/ndri/sidekickicons
- **Author:** Andrea Debnar (@ndri)
- **License:** MIT License
- **Copyright:** © 2021 Andrea Debnar

## Disclaimer

I've done my best to document everything accurately - however, there might be
mistakes. If you see them, or opportunities to improve, please open an issue. This is an open-source project with no warranties or guarantees.

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
into commercial software), please contact the author.

### Full License

You can view the full license text at:
https://creativecommons.org/licenses/by-nc-sa/4.0/

### Copyright

© 2025 Adam Knowles. All rights reserved.
