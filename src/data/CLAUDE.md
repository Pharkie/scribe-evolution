# Frontend Source - CLAUDE.md

<system_context>
Frontend SOURCE files: Alpine.js + Tailwind CSS components.
These files are built and copied to /data/ directory.
Styling uses Tailwind utility classes with minimal custom CSS (font-face + a keyframes animation in `src/source-css-files`).
</system_context>

<critical_notes>

- This is the SOURCE directory for frontend development
- After changes: ALWAYS run `npm run build` (copies to /data/)
- Use Alpine.js reactivity - NO custom solutions
- Never edit /data/ directly - changes will be lost
- Do not add semantic CSS classes; prefer Tailwind utilities. Minimal custom CSS is allowed only for fonts and isolated animations.
  </critical_notes>

<paved_path>
Alpine.js Patterns:

1. Use Alpine stores for state management
2. Use x-data for component-level state
3. Use x-effect for side effects, $watch for reactive updates
4. Conditional rendering with x-if/x-show

Build Process:
Source files here → npm run build → Built files in /data/
</paved_path>

<patterns>
// Alpine store pattern
Alpine.store('config', {
    deviceOwner: '',
    wifiConnected: false,
    
    async loadConfig() {
        const response = await fetch('/api/config');
        if (!response.ok) {
            const error = await response.json();
            this.showError(error.message);
            return;
        }
        const data = await response.json();
        this.deviceOwner = data.deviceOwner;
        this.wifiConnected = data.wifiConnected;
    },
    
    showError(message) {
        console.error('Config error:', message);
        // Show user-visible error
    }
});

// Component pattern with validation

<div x-data="{ 
    ssid: '', 
    password: '',
    async submitWifi() {
        const response = await fetch('/api/wifi', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ ssid: this.ssid, password: this.password })
        });
        
        if (!response.ok) {
            const error = await response.json();
            alert(error.message);
            return;
        }
        
        // Success - no JSON to parse
        this.ssid = '';
        this.password = '';
    }
}">

// Conditional rendering
<template x-if="$store.config.wifiConnected">

<div class="text-green-600">Connected</div>
</template>
</patterns>

<common_tasks>
Adding new page/component:

1. Create HTML file with Alpine.js patterns
2. Use Alpine stores for shared state
3. Handle both success (200 empty) and error responses
4. Test with mock server first
5. Run npm run build to deploy
   </common_tasks>

<hatch>
Mock Server Development:
Use mock-server for rapid frontend iteration without ESP32 hardware rebuilds.
Run `npm start` in mock-server/ directory.
</hatch>

<fatal_implications>

- Edit /data/ directly = Changes lost on next build
- Custom reactivity = Ignore Alpine's built-in solutions
- Fallback values = Hide real backend errors
- Skip npm run build = Frontend changes not deployed
  </fatal_implications>
