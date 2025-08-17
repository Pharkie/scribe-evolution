# HTML Partials Linting Configuration

## Overview

HTML partial files in `/data/html/partials/` are fragments that get loaded
dynamically into complete HTML documents. They don't require DOCTYPE
declarations and have special linting rules.

## Configuration

### `.htmlhintrc`

Global HTML linting rules with `doctype-first: false` to allow partials without
DOCTYPE.

### `.vscode/settings.json`

- Disables script and style validation for HTML files
- Sets `htmlhint.options.doctype-first: false`
- Associates partial files with HTML language mode

### Inline Directives

Partial files include `htmlhint-disable doctype-first` comments to suppress
DOCTYPE warnings.

## File Structure

```
data/html/partials/
├── led-effects-config.html    # LED effects configuration form
└── (future partials...)       # Additional UI components
```

## Usage

Partials are loaded via JavaScript:

```javascript
const response = await fetch("/html/partials/led-effects-config.html");
const html = await response.text();
container.innerHTML = html;
```

## Best Practices

1. Include header comment explaining the partial's purpose
2. Add `htmlhint-disable doctype-first` for clean linting
3. Use semantic HTML structure even without DOCTYPE
4. Test partial integration with main document
