# JavaScript Frontend Requirements

## Code Standards

- Use async/await instead of promises or callbacks
- Always wrap API calls in try/catch blocks
- Use ES6+ features (arrow functions, template literals, destructuring)
- Use camelCase and descriptive function names

## API Integration

```javascript
const response = await fetch("/api/endpoint", {
  method: "POST",
  headers: { "Content-Type": "application/json" },
  body: JSON.stringify(data),
});
```

- Always check `response.ok` before parsing JSON
- Parse error responses and show user-friendly messages

## User Feedback

- Use `showMessage(message, type)` for feedback: 'success', 'error', 'info', 'warning'
- Show loading indicators for operations >100ms
- Avoid duplicate toast messages for same action

## Form Handling

- Always call `event.preventDefault()` in form handlers
- Validate inputs client-side before API calls
- Use helper functions like `collectFormData()`
- Clear forms appropriately after successful submissions

## LED Effect Integration

- Support configurable effect durations (default 10s testing, 3s confirmations)
- Allow color parameters (red, green, blue, etc.)
- Support effects: simple_chase, rainbow, twinkle, chase, pulse, matrix
- Trigger LED effects for important actions (green chase on settings save)

## Settings Management

- Maintain current config state in memory
- Validate all fields before submission
- Support tabbed/sectioned settings interface

## Performance Best Practices

- Debounce rapid user inputs (typing, button clicks)
- Minimize DOM queries and updates
- Use event delegation for dynamic content
- Clean up event listeners and timeouts

## Build Integration

- Source files in `src/js/`, builds to `data/js/`
- Code gets minified via Terser for production
- Organize code in focused modules
- Minimize external dependencies, prefer vanilla JS
