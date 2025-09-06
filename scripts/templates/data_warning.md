Do not edit files in this directory.

This `data/` folder contains built assets that are served by the device (ESP32 filesystem). Files here are overwritten by the frontend build and optimization steps.

Source of truth:

- HTML: `src/web-static/`
- JavaScript: `src/js-source/`
- CSS (Tailwind): `src/css-source/`

How to update UI/JS/CSS:

1. Make changes in the appropriate `src/` directory.
2. Run `npm run build` to regenerate files in `data/`.

Notes:

- Any manual changes in `data/` will be clobbered on the next build.
- Keep `data/` minimal and production-ready; avoid adding docs or temp files here.
