# Settings UI JavaScript Instructions

Instructions specific to settings.html, settings JavaScript modules, and the
unbidden ink interface.

## File Pattern Coverage

- `data/html/settings.html`
- `src/js/settings-*.js` (all settings modules)
- Settings-related API endpoints and configurations

## JavaScript Module Architecture

### 4-Module System Structure

- **settings-api.js**: HTTP requests and server communication only
- **settings-ui.js**: DOM manipulation, form handling, visual updates
- **settings-led.js**: LED-specific functionality and demo effects
- **settings-main.js**: Coordination, initialization, event listener setup

### Module Export Pattern

```javascript
window.ModuleName = {
  functionName,
  anotherFunction,
};
```

## Event Handling Requirements

### Modern Event Listeners ONLY

- **NEVER** use inline HTML handlers (`onclick`, `oninput`, `onchange`)
- **ALWAYS** use `addEventListener()` with proper cleanup
- **ALWAYS** call `removeAttribute()` after adding listeners
- Group event listener setup in dedicated functions in `settings-main.js`

### Event Handler Setup Pattern

```javascript
function setupSpecificHandlers() {
  const element = document.getElementById("element-id");
  if (element) {
    element.addEventListener("event", function (e) {
      e.preventDefault();
      // Handle event
    });
    element.removeAttribute("onclick"); // Clean up inline handler
  }
}
```

## Unbidden Ink Time Range System

### Special "All Day" Handling

- Support **both** `0-0` and `0-24` as "All Day" settings
- Normalize `0-24` to `0-0` for consistency in internal logic
- Display format: "00:00 (All Day)" and "24:00 (All Day)" for clarity
- Frequency display: "all day long" instead of "from X to Y"

### Time Range Slider Logic

```javascript
// Special case handling in updateTimeRange()
if ((startVal === 0 && endVal === 0) || (startVal === 0 && endVal === 24)) {
  // This is valid - full day operation
  // Normalize 0-24 to 0-0 for consistency
  if (endVal === 24) {
    endSlider.value = 0;
    endVal = 0;
  }
}
```

### Click Area Collision Prevention

- Call `updateClickAreas()` with 100ms delay during initialization
- Prevent slider handle click areas from overlapping
- Update click areas after any slider value changes
- Calculate boundaries based on slider positions and midpoint

## Frequency Slider Integration

### Slider Position Mapping

```javascript
// Map slider positions [0-7] to frequency values
const frequencyValues = [15, 30, 45, 60, 120, 240, 360, 480];
```

### Configuration Loading Requirements

- Call `updateSliderFromFrequency(minutes)` during config loading
- Update both hidden input value AND visual slider position
- Ensure frequency display updates to match loaded values
- Handle edge cases for unmapped frequency values

## Custom Prompt Preset Matching

### Preset Text Arrays (Must Match HTML Exactly)

```javascript
const presetPrompts = [
  "Generate a short, inspiring quote about creativity, technology, or daily life. Keep it under 200 characters.",
  "Generate a fun fact under 200 characters about BBC Doctor Who - the characters, episodes, behind-the-scenes trivia, or the show's history that is esoteric and only 5% of fans might know.",
  "Write a short, humorous observation about everyday life or a witty one-liner. Keep it light and under 200 characters.",
  "Generate a short creative writing prompt, mini-story, or poetic thought. Be imaginative and keep under 250 characters.",
];
```

### Visual Highlighting System

- Add/remove CSS classes: `bg-purple-100`, `dark:bg-purple-900/40`, `ring-2`,
  `ring-purple-400`
- Clear highlights when user types custom content
- Check for exact matches on config load and text changes
- Use `setTimeout()` delays for DOM-dependent highlighting operations

## DOM Timing and Initialization

### Required Initialization Delays

```javascript
// Click areas need 100ms delay
setTimeout(() => {
  updateClickAreas();
}, 100);

// Preset matching needs 150ms delay
setTimeout(() => {
  highlightMatchingPreset(customPrompt);
}, 150);
```

### Event Handler Parameter Passing

```javascript
// Time range sliders need type parameter
timeStartSlider.addEventListener("input", function () {
  window.SettingsUI.updateTimeRange(this, "start");
});
```

## Visual Consistency Requirements

### Time Display Formats

- **24-hour format** for time displays: "00:00", "23:00"
- **12-hour format** for frequency descriptions: "12 am", "11 pm"
- **Special case**: "00:00 (All Day)" for start and end when 0-0
- **Next day indicator**: "00:00 (next day)" when end time is 24

### Frequency Description Patterns

```javascript
if (startHour === 0 && endHour === 0) {
  text = `Around every ${minutes} minutes, all day long`;
} else {
  text = `Around every ${minutes} minutes from ${startTime} to ${endTime} each day`;
}
```

## Common Pitfalls to Avoid

1. **Don't** mix inline handlers with event listeners
2. **Don't** forget to call `removeAttribute()` after adding listeners
3. **Don't** forget initialization delays for DOM-dependent operations
4. **Don't** assume frequency values map directly to slider positions
5. **Don't** forget to handle both 0-0 and 0-24 as "All Day"
6. **Don't** skip preset text exact matching - must be character-perfect
7. **Don't** forget to update both input values and slider positions
8. **Don't** skip click area updates after slider changes

## Testing Requirements

Before marking settings UI work complete:

- ✅ Test frequency slider loads saved values correctly
- ✅ Test time range displays show correct format for 0-0 and normal ranges
- ✅ Test custom prompt preset matching works on page load
- ✅ Test all event listeners work without inline handlers
- ✅ Test click areas don't overlap when handles are close
- ✅ Test "All Day" displays correctly in both time display and frequency text
- ✅ Test all visual highlighting appears and clears appropriately
