# Settings UI JavaScript Instructions

## File Coverage
- `data/html/settings.html`
- `src/js/settings-*.js` modules
- Settings-related API endpoints

## Module Architecture
- **settings-api.js**: HTTP requests and server communication
- **settings-ui.js**: DOM manipulation, form handling, visual updates  
- **settings-led.js**: LED-specific functionality and demo effects
- **settings-main.js**: Coordination, initialization, event listener setup

Export pattern: `window.ModuleName = { functionName, anotherFunction };`

## Event Handling Requirements

### Modern Event Listeners ONLY
- **NEVER** use inline HTML handlers (`onclick`, `oninput`, `onchange`)
- **ALWAYS** use `addEventListener()` with proper cleanup
- Call `removeAttribute()` after adding listeners
- Group setup in dedicated functions in `settings-main.js`

Pattern:
```javascript
function setupSpecificHandlers() {
  const element = document.getElementById("element-id");
  if (element) {
    element.addEventListener("event", function (e) {
      e.preventDefault();
      // Handle event
    });
    element.removeAttribute("onclick");
  }
}
```

## Unbidden Ink Time Range System

### Special "All Day" Handling
- Support both `0-0` and `0-24` as "All Day" settings
- Normalize `0-24` to `0-0` for consistency
- Display format: "00:00 (All Day)" and "24:00 (All Day)"
- Frequency display: "all day long" instead of "from X to Y"

### Click Area Collision Prevention
- Call `updateClickAreas()` with 100ms delay during initialization
- Update click areas after any slider value changes

## Frequency Slider Integration
- Map slider positions [0-7] to frequency values: [15, 30, 45, 60, 120, 240, 360, 480]
- Call `updateSliderFromFrequency(minutes)` during config loading
- Update both hidden input value AND visual slider position

## Custom Prompt Preset Matching
- Preset text arrays must match HTML exactly
- Add/remove CSS classes: `bg-purple-100`, `dark:bg-purple-900/40`, `ring-2`, `ring-purple-400`
- Clear highlights when user types custom content
- Use 150ms delay for DOM-dependent highlighting operations

## DOM Timing and Initialization
- Click areas need 100ms delay: `setTimeout(() => { updateClickAreas(); }, 100);`
- Preset matching needs 150ms delay for DOM-dependent operations
- Time range sliders need type parameter: `updateTimeRange(this, "start")`

## Visual Consistency Requirements
- **24-hour format** for time displays: "00:00", "23:00"
- **12-hour format** for frequency descriptions: "12 am", "11 pm"
- **Special case**: "00:00 (All Day)" for start and end when 0-0

## Testing Checklist
- ✅ Frequency slider loads saved values correctly
- ✅ Time range displays show correct format for 0-0 and normal ranges
- ✅ Custom prompt preset matching works on page load
- ✅ All event listeners work without inline handlers
- ✅ Click areas don't overlap when handles are close
- ✅ "All Day" displays correctly in time display and frequency text
