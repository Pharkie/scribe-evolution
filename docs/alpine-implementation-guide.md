# Alpine.js Settings Implementation Guide

## Overview

This guide documents the Alpine.js implementation of Scribe ESP32-C3 settings forms, providing a modern reactive alternative to the vanilla JavaScript implementation.

## Architecture

### File Structure
```
src/js/
├── settings-alpine-store.js    # Alpine.js reactive data store
└── settings-api.js             # Existing API layer (unchanged)

data/html/
├── settings.html               # Original vanilla JS implementation
└── settings-alpine.html        # New Alpine.js implementation

data/js/
├── alpine.min.js              # Alpine.js library (44KB)
├── settings-alpine.min.js     # Compiled Alpine store + API (6.4KB)
└── settings.min.js            # Original vanilla JS bundle (29KB)
```

### Build Commands

```bash
# Build Alpine.js version
npm run build-js-settings-alpine

# Build original vanilla JS version  
npm run build-js-settings
```

## Alpine.js Store Architecture

The `settings-alpine-store.js` provides a reactive data store that wraps the existing API layer:

```javascript
function initializeSettingsStore() {
    return {
        // Loading states
        loading: false,
        saving: false,
        
        // Reactive configuration data
        config: {
            device: { owner: '', timezone: '' },
            wifi: { ssid: '', password: '', connect_timeout: 15000 },
            mqtt: { server: '', port: 1883, username: '', password: '' },
            unbiddenInk: { enabled: false, startHour: 8, endHour: 22, frequencyMinutes: 120, prompt: '' },
            buttons: { /* ... */ }
        },
        
        // Computed properties for reactive UI updates
        get timeRangeDisplay() {
            const start = this.config.unbiddenInk.startHour;
            const end = this.config.unbiddenInk.endHour;
            return start === 0 && (end === 0 || end === 24) ? 'All Day' : 
                   `${this.formatHour(start)} - ${this.formatHour(end)}`;
        },
        
        get frequencyDisplay() {
            // Automatically updates when frequency or time range changes
            const minutes = this.config.unbiddenInk.frequencyMinutes;
            const start = this.config.unbiddenInk.startHour;
            const end = this.config.unbiddenInk.endHour;
            return `Every ${this.formatFrequency(minutes)} ${this.formatTimeRange(start, end)}`;
        },
        
        // API integration methods
        async loadConfiguration() { /* Uses existing SettingsAPI */ },
        async saveConfiguration() { /* Uses existing SettingsAPI */ }
    };
}
```

## HTML Template Patterns

### 1. Basic Data Binding

```html
<!-- Two-way data binding with x-model -->
<input type="text" x-model="config.device.owner" class="input-standard">

<!-- Reactive display with x-text -->
<span x-text="config.device.owner"></span>

<!-- Reactive attributes -->
<input :value="Math.floor(config.wifi.connect_timeout / 1000)" 
       @input="config.wifi.connect_timeout = parseInt($el.value) * 1000">
```

### 2. Conditional Rendering

```html
<!-- Show/hide sections based on state -->
<div x-show="config.unbiddenInk.enabled" x-transition class="space-y-6">
    <!-- Unbidden Ink settings only visible when enabled -->
</div>

<!-- Loading states -->
<div x-show="loading" class="loading-card" x-transition>
    <div class="loading-spinner"></div>
    <span>Loading configuration...</span>
</div>
```

### 3. Template Iteration

```html
<!-- Generate multiple similar elements -->
<template x-for="buttonNum in [1, 2, 3, 4]" :key="buttonNum">
    <div class="button-config">
        <h3 x-text="`Button ${buttonNum} Configuration`"></h3>
        
        <select :id="`button${buttonNum}-short`"
                x-model="config.buttons[`button${buttonNum}`].shortAction">
            <option value="">No Action</option>
            <option value="print_status">Print Status</option>
        </select>
    </div>
</template>
```

### 4. Reactive Styling

```html
<!-- Dynamic styles based on state -->
<div class="dual-range-active"
     :style="`left: ${timeRangeStyle.left}; width: ${timeRangeStyle.width};`">
</div>

<!-- Conditional CSS classes -->
<button :disabled="saving"
        :class="{ 'opacity-50 cursor-not-allowed': saving }"
        class="btn-primary">
    <span x-show="!saving">💾 Save Settings</span>
    <span x-show="saving">Saving...</span>
</button>
```

### 5. Event Handling

```html
<!-- Form submission -->
<form @submit.prevent="saveConfiguration()" class="space-y-6">

<!-- Click handlers -->
<button @click="window.location.href = '/'" class="back-button">
    Back
</button>

<!-- Input events -->
<input type="range" 
       x-model.number="config.unbiddenInk.startHour"
       @input="/* automatically triggers reactive updates */">
```

## Key Alpine.js Directives Used

| Directive | Purpose | Example |
|---|---|---|
| `x-data` | Define component scope | `<main x-data="initializeSettingsStore()">` |
| `x-init` | Run code on component init | `<main x-init="loadConfiguration()">` |
| `x-model` | Two-way data binding | `<input x-model="config.device.owner">` |
| `x-text` | Reactive text content | `<span x-text="timeRangeDisplay"></span>` |
| `x-show` | Conditional visibility | `<div x-show="config.unbiddenInk.enabled">` |
| `x-for` | Template iteration | `<template x-for="item in items">` |
| `x-transition` | Smooth show/hide animations | `<div x-show="loading" x-transition>` |
| `:attribute` | Dynamic attributes | `:disabled="saving"` |
| `@event` | Event listeners | `@click="saveConfiguration()"` |

## Reactive Data Flow

1. **Initial Load**: `loadConfiguration()` populates `config` from API
2. **User Interaction**: Form inputs update `config` via `x-model`
3. **Automatic Updates**: Computed properties recalculate when dependencies change
4. **UI Updates**: `x-text`, `:style` directives update DOM automatically  
5. **Form Submission**: `saveConfiguration()` sends current `config` state to API

## Complex Features Implementation

### Time Range Sliders

The time range sliders demonstrate Alpine.js handling complex interdependent UI:

```javascript
// Computed property automatically handles visual track updates
get timeRangeStyle() {
    const start = this.config.unbiddenInk.startHour;
    const end = this.config.unbiddenInk.endHour;
    
    if (start === 0 && (end === 0 || end === 24)) {
        return { left: '0%', width: '100%' };
    }
    
    const startPercent = (start / 24) * 100;
    const endPercent = (end / 24) * 100;
    
    return {
        left: `${startPercent}%`,
        width: `${endPercent - startPercent}%`
    };
}
```

```html
<!-- Reactive visual track -->
<div class="dual-range-active"
     :style="`left: ${timeRangeStyle.left}; width: ${timeRangeStyle.width};`">
</div>

<!-- Dual range inputs -->
<input type="range" min="0" max="24" step="1"
       x-model.number="config.unbiddenInk.startHour"
       class="dual-range-input dual-range-start">

<input type="range" min="0" max="24" step="1"
       x-model.number="config.unbiddenInk.endHour"  
       class="dual-range-input dual-range-end">

<!-- Reactive time display -->
<span x-text="formatHour(config.unbiddenInk.startHour)"></span>
<span x-text="timeRangeDisplay"></span>
<span x-text="formatHour(config.unbiddenInk.endHour)"></span>
```

### Button Configuration Template

Template iteration eliminates repetitive HTML:

```html
<template x-for="buttonNum in [1, 2, 3, 4]" :key="buttonNum">
    <div class="p-4 bg-orange-50 rounded-xl">
        <h3 x-text="`Button ${buttonNum} Configuration`"></h3>
        
        <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
            <select :id="`button${buttonNum}-short`"
                    x-model="config.buttons[`button${buttonNum}`].shortAction">
                <option value="">No Action</option>
                <option value="print_status">Print Status</option>
                <!-- ... more options ... -->
            </select>
            
            <input :id="`button${buttonNum}-short-mqtt`"
                   x-model="config.buttons[`button${buttonNum}`].shortMqttTopic"
                   placeholder="Optional MQTT topic">
        </div>
    </div>
</template>
```

## Form Validation

Alpine.js enables reactive validation:

```javascript
// In the store
validateForm() {
    this.validation.errors = {};
    
    const requiredFields = ['device.owner', 'wifi.ssid'];
    if (this.config.unbiddenInk.enabled) {
        requiredFields.push('apis.chatgptApiToken');
    }
    
    requiredFields.forEach(fieldName => {
        const value = this.getNestedValue(fieldName);
        if (!value || value.trim() === '') {
            this.validation.errors[fieldName] = 'This field is required';
        }
    });
    
    this.validation.isValid = Object.keys(this.validation.errors).length === 0;
    return this.validation.isValid;
}

async saveConfiguration() {
    if (!this.validateForm()) {
        this.showMessage('Please fix the errors in the form', 'error');
        return;
    }
    
    // Proceed with save...
}
```

## Performance Considerations

### Bundle Size Optimization
- Alpine.js: 44KB (reasonable for ESP32-C3 4MB flash)
- Store + API: 6.4KB (much smaller than original 29KB vanilla JS)
- Total: 50.4KB vs original 29KB (+21KB acceptable increase)

### Memory Usage
- Reactive system uses minimal memory overhead
- No manual DOM queries or event listener management
- Computed properties only recalculate when dependencies change

### ESP32-C3 Compatibility
- Tested bundle size well within ESP32-C3 4MB flash constraints
- Reactive updates perform efficiently on embedded hardware
- No external dependencies beyond Alpine.js core

## Migration Strategy

### For New Projects
1. Use Alpine.js implementation directly (`settings-alpine.html`)
2. Build with `npm run build-js-settings-alpine`
3. Reference Alpine.js patterns for new forms

### For Existing Projects
1. Keep vanilla JS implementation as fallback
2. Gradually migrate complex forms to Alpine.js patterns
3. A/B test performance on target hardware
4. Full migration when confidence is established

## Troubleshooting

### Common Issues

1. **Alpine.js not loading**: Ensure `alpine.min.js` is included before application scripts
2. **Data not binding**: Check `x-data` is properly initialized on parent element  
3. **Template iteration not working**: Ensure proper `:key` attribute in `x-for`
4. **Form submission failing**: Verify `@submit.prevent` and validation logic

### Debug Tools

```javascript
// Access Alpine store from browser console
const main = document.querySelector('main[x-data]');
console.log(main.__x.$data); // View current store state
```

## Conclusion

The Alpine.js implementation provides a modern, maintainable alternative to vanilla JavaScript DOM manipulation while preserving full functionality and ESP32-C3 compatibility. The reactive data binding significantly reduces code complexity and improves the developer experience.