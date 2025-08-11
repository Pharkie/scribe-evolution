# Scribe CSS Build Setup

## Overview

This project has been configured to use a local Tailwind CSS build instead of
the CDN version for production use.

## Issue Resolution

The build was initially hanging due to Node.js version conflicts:

- System had Node v12.19.0 in `/usr/local/bin/node`
- nvm had Node v24.4.1 available but not loaded by default
- Accidentally installed Tailwind CSS v4.x which has different CLI syntax
- **Solution**: Use nvm to load Node v24.4.1 and downgrade to Tailwind CSS
  v3.4.1

## Files Created

- `package.json` - Node.js package configuration with Tailwind CSS v3.4.1
  dependency
- `tailwind.config.js` - Tailwind CSS configuration with dark mode support
- `src/input.css` - Input CSS file with Tailwind directives and custom
  components
- `data/css/tailwind.css` - Generated production CSS file (26KB minified)
- `.nvmrc` - Specifies Node.js v24.4.1 for consistency

## Build Commands

All commands automatically load the correct Node.js version via nvm:

- `npm run build-css` - Build production CSS
- `npm run build-css-prod` - Build production CSS (alias)
- `npm run watch-css` - Watch for changes and rebuild automatically
- `npm run build-css-dev` - Watch mode for development

## Node.js Version Management

- **Required**: Node.js v24.4.1 (managed via nvm)
- **Issue**: System default Node v12.19.0 was incompatible
- **Solution**: All npm scripts automatically run
  `source ~/.nvm/nvm.sh && nvm use 24.4.1`
- **Verification**: Run `node --version` after sourcing nvm should show v24.4.1

## Troubleshooting

If build commands hang or show "Unexpected token '?'" errors:

1. Check Node version:
   `source ~/.nvm/nvm.sh && nvm use 24.4.1 && node --version`
2. Verify Tailwind version: `npm list tailwindcss` (should be 3.4.1, not 4.x)
3. If wrong version:
   `npm uninstall tailwindcss && npm install tailwindcss@3.4.1`

## Changes Made

1. **HTML Files Updated**: Replaced CDN links with local CSS in:

   - `data/html/index.html`
   - `data/html/diagnostics.html`

2. **Production Ready**: No more CDN dependency warnings

3. **Dark Mode**: Fully supported with 35+ dark mode selectors included

4. **File Size**: Optimized 26KB minified CSS vs full CDN version

## Rebuilding CSS

When you modify the HTML files or add new Tailwind classes:

```bash
npm run build-css
```

## Development Workflow

For active development with auto-rebuild:

```bash
npm run watch-css
```

## Custom Components

The following custom component classes are available:

- `.btn-primary` - Primary button styling
- `.btn-secondary` - Secondary button styling
- `.input-field` - Form input styling
- `.card` - Card container styling

## Notes

- The generated CSS only includes classes that are actually used in the HTML
  files
- Dark mode is configured with the 'class' strategy (35 selectors included)
- All build artifacts are added to `.gitignore`
- `.nvmrc` file ensures consistent Node.js version across environments
