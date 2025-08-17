---
applyTo: "src/js/**/*.js"
---

# JavaScript Frontend Requirements

When working with JavaScript files for the Scribe web interface, follow these
guidelines:

## Code Standards

1. **Async/Await Pattern** - Use async/await instead of promises or callbacks
2. **Error Handling** - Always wrap API calls in try/catch blocks
3. **ES6+ Features** - Use modern JavaScript (arrow functions, template
   literals, destructuring)
4. **Function Naming** - Use camelCase and descriptive names

## API Integration

1. **Fetch Patterns** - Use consistent fetch structure:
   ```javascript
   const response = await fetch("/api/endpoint", {
     method: "POST",
     headers: { "Content-Type": "application/json" },
     body: JSON.stringify(data),
   });
   ```
2. **Response Handling** - Always check `response.ok` before parsing JSON
3. **Error Messages** - Parse error responses and show user-friendly messages

## User Feedback

1. **Toast Messages** - Use `showMessage(message, type)` for all user feedback:
   - Types: 'success', 'error', 'info', 'warning'
   - Keep messages concise and actionable
2. **Loading States** - Show loading indicators for operations >100ms
3. **Avoid Duplicate Messages** - Don't show multiple toasts for same action
   (recent fix: removed duplicate "Turning off LEDs" messages)

## Form Handling

1. **preventDefault** - Always call `event.preventDefault()` in form handlers
2. **Validation** - Validate inputs client-side before API calls
3. **Data Collection** - Use helper functions like `collectFormData()`
4. **Reset Forms** - Clear forms appropriately after successful submissions

## LED Effect Integration

1. **Duration Handling** - Support configurable effect durations (default 10s
   for testing, 3s for confirmations)
2. **Color Support** - Allow color parameters (red, green, blue, etc.)
3. **Effect Names** - Support all available effects: simple_chase, rainbow,
   twinkle, chase, pulse, matrix
4. **Success Feedback** - Trigger LED effects for important actions (green chase
   on settings save)

## Settings Management

1. **Configuration Objects** - Maintain current config state in memory
2. **Validation** - Validate all fields before submission
3. **Section Handling** - Support tabbed/sectioned settings interface
4. **Auto-save** - Consider auto-save for non-critical settings

## Recent Patterns to Follow

1. **Responsive Design Support** - JavaScript should work across all breakpoints
2. **Dark Mode Compatibility** - Support theme switching
3. **Quick Actions** - Implement quick action buttons with proper grid layouts
4. **Settings Persistence** - Handle configuration save/load cycles
5. **LED Visual Feedback** - Integrate LED effects for user actions

## Performance Best Practices

1. **Debouncing** - Debounce rapid user inputs (typing, button clicks)
2. **Efficient DOM Manipulation** - Minimize DOM queries and updates
3. **Event Delegation** - Use event delegation for dynamic content
4. **Memory Management** - Clean up event listeners and timeouts

## Build Integration

1. **Source Files** - Keep source in `src/js/`, builds to `data/js/`
2. **Minification** - Code gets minified via Terser for production
3. **Modular Structure** - Organize code in focused modules
4. **Dependencies** - Minimize external dependencies, prefer vanilla JS

## Testing Considerations

1. **Error Simulation** - Handle network failures gracefully
2. **Edge Cases** - Test with malformed responses and timeouts
3. **Cross-browser** - Ensure compatibility with modern browsers
4. **Mobile Testing** - Verify touch interactions and responsive behavior
