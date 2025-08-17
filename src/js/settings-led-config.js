/**
 * @file settings-led-config.js
 * @brief LED configuration module for settings page
 * @description Handles all LED-related configuration including autonomous per-effect settings
 */

/**
 * Helper function to convert color values for HTML color inputs
 * Handles both #RRGGBB and #RRGGBBAA formats
 */
function colorToHtml(color, defaultColor) {
    const colorValue = color || defaultColor;
    // Remove alpha channel if present (HTML color input doesn't support alpha)
    return colorValue.length > 7 ? colorValue.substring(0, 7) : colorValue;
}

/**
 * Helper function to convert HTML color input to RGBA format
 */
function colorToRgba(color) {
    // Add full opacity alpha channel if not present
    return color.length === 7 ? color + 'ff' : color;
}

/**
 * Populate LED configuration form fields with current settings
 * @param {Object} config - Full configuration object
 */
function populateLedForm(config) {
    if (!config.leds) return;

    // Hardware settings
    document.getElementById('led-pin').value = config.leds.pin || 4;
    document.getElementById('led-count').value = config.leds.count || 30;
    document.getElementById('led-brightness').value = config.leds.brightness || 64;
    document.getElementById('led-refresh-rate').value = config.leds.refreshRate || 60;
    
    // Populate autonomous per-effect configurations
    const effects = config.leds.effects || {};
    
    // Chase Single
    const chaseSingle = effects.chaseSingle || {};
    document.getElementById('chase-single-speed').value = chaseSingle.speed || 5;
    document.getElementById('chase-single-trail-length').value = chaseSingle.trailLength || 15;
    document.getElementById('chase-single-trail-fade').value = chaseSingle.trailFade || 15;
    document.getElementById('chase-single-color').value = colorToHtml(chaseSingle.defaultColor, '#0062ff');
    
    // Chase Multi
    const chaseMulti = effects.chaseMulti || {};
    document.getElementById('chase-multi-speed').value = chaseMulti.speed || 2;
    document.getElementById('chase-multi-trail-length').value = chaseMulti.trailLength || 20;
    document.getElementById('chase-multi-trail-fade').value = chaseMulti.trailFade || 20;
    document.getElementById('chase-multi-color-spacing').value = chaseMulti.colorSpacing || 12;
    document.getElementById('chase-multi-color1').value = colorToHtml(chaseMulti.color1, '#ff9900');
    document.getElementById('chase-multi-color2').value = colorToHtml(chaseMulti.color2, '#008f00');
    document.getElementById('chase-multi-color3').value = colorToHtml(chaseMulti.color3, '#78cffe');
    
    // Matrix
    const matrix = effects.matrix || {};
    document.getElementById('matrix-speed').value = matrix.speed || 3;
    document.getElementById('matrix-drops').value = matrix.drops || 5;
    document.getElementById('matrix-background-fade').value = matrix.backgroundFade || 64;
    document.getElementById('matrix-trail-fade').value = matrix.trailFade || 32;
    document.getElementById('matrix-brightness-fade').value = matrix.brightnessFade || 40;
    document.getElementById('matrix-color').value = colorToHtml(matrix.defaultColor, '#009100');
    
    // Twinkle
    const twinkle = effects.twinkle || {};
    document.getElementById('twinkle-density').value = twinkle.density || 8;
    document.getElementById('twinkle-fade-speed').value = twinkle.fadeSpeed || 5;
    document.getElementById('twinkle-min-brightness').value = twinkle.minBrightness || 50;
    document.getElementById('twinkle-max-brightness').value = twinkle.maxBrightness || 255;
    document.getElementById('twinkle-color').value = colorToHtml(twinkle.defaultColor, '#ffffff');
    
    // Pulse
    const pulse = effects.pulse || {};
    document.getElementById('pulse-speed').value = pulse.speed || 4;
    document.getElementById('pulse-min-brightness').value = pulse.minBrightness || 0;
    document.getElementById('pulse-max-brightness').value = pulse.maxBrightness || 255;
    document.getElementById('pulse-wave-frequency').value = pulse.waveFrequency || 0.05;
    document.getElementById('pulse-color').value = colorToHtml(pulse.defaultColor, '#ff00f2');
    
    // Rainbow
    const rainbow = effects.rainbow || {};
    document.getElementById('rainbow-speed').value = rainbow.speed || 2.0;
    document.getElementById('rainbow-saturation').value = rainbow.saturation || 255;
    document.getElementById('rainbow-brightness').value = rainbow.brightness || 255;
    document.getElementById('rainbow-hue-step').value = rainbow.hueStep || 2.5;
}

/**
 * Collect LED configuration data from form fields
 * @returns {Object} LED configuration object
 */
function collectLedFormData() {
    return {
        pin: parseInt(document.getElementById('led-pin').value),
        count: parseInt(document.getElementById('led-count').value),
        brightness: parseInt(document.getElementById('led-brightness').value),
        refreshRate: parseInt(document.getElementById('led-refresh-rate').value),
        effects: {
            chaseSingle: {
                speed: parseInt(document.getElementById('chase-single-speed').value),
                trailLength: parseInt(document.getElementById('chase-single-trail-length').value),
                trailFade: parseInt(document.getElementById('chase-single-trail-fade').value),
                defaultColor: colorToRgba(document.getElementById('chase-single-color').value)
            },
            chaseMulti: {
                speed: parseInt(document.getElementById('chase-multi-speed').value),
                trailLength: parseInt(document.getElementById('chase-multi-trail-length').value),
                trailFade: parseInt(document.getElementById('chase-multi-trail-fade').value),
                colorSpacing: parseInt(document.getElementById('chase-multi-color-spacing').value),
                color1: colorToRgba(document.getElementById('chase-multi-color1').value),
                color2: colorToRgba(document.getElementById('chase-multi-color2').value),
                color3: colorToRgba(document.getElementById('chase-multi-color3').value)
            },
            matrix: {
                speed: parseInt(document.getElementById('matrix-speed').value),
                drops: parseInt(document.getElementById('matrix-drops').value),
                backgroundFade: parseInt(document.getElementById('matrix-background-fade').value),
                trailFade: parseInt(document.getElementById('matrix-trail-fade').value),
                brightnessFade: parseInt(document.getElementById('matrix-brightness-fade').value),
                defaultColor: colorToRgba(document.getElementById('matrix-color').value)
            },
            twinkle: {
                density: parseInt(document.getElementById('twinkle-density').value),
                fadeSpeed: parseInt(document.getElementById('twinkle-fade-speed').value),
                minBrightness: parseInt(document.getElementById('twinkle-min-brightness').value),
                maxBrightness: parseInt(document.getElementById('twinkle-max-brightness').value),
                defaultColor: document.getElementById('twinkle-color').value // Twinkle doesn't use alpha
            },
            pulse: {
                speed: parseInt(document.getElementById('pulse-speed').value),
                minBrightness: parseInt(document.getElementById('pulse-min-brightness').value),
                maxBrightness: parseInt(document.getElementById('pulse-max-brightness').value),
                waveFrequency: parseFloat(document.getElementById('pulse-wave-frequency').value),
                defaultColor: colorToRgba(document.getElementById('pulse-color').value)
            },
            rainbow: {
                speed: parseFloat(document.getElementById('rainbow-speed').value),
                saturation: parseInt(document.getElementById('rainbow-saturation').value),
                brightness: parseInt(document.getElementById('rainbow-brightness').value),
                hueStep: parseFloat(document.getElementById('rainbow-hue-step').value)
            }
        }
    };
}

/**
 * Validate LED configuration data
 * @param {Object} ledConfig - LED configuration object
 * @returns {Object} Validation result with isValid boolean and error message
 */
function validateLedConfig(ledConfig) {
    // Hardware validation
    if (ledConfig.pin < 0 || ledConfig.pin > 10) {
        return { isValid: false, error: 'LED pin must be between 0 and 10 (ESP32-C3 compatible)' };
    }
    
    if (ledConfig.count < 1 || ledConfig.count > 300) {
        return { isValid: false, error: 'LED count must be between 1 and 300' };
    }
    
    if (ledConfig.brightness < 0 || ledConfig.brightness > 255) {
        return { isValid: false, error: 'LED brightness must be between 0 and 255' };
    }
    
    if (ledConfig.refreshRate < 10 || ledConfig.refreshRate > 120) {
        return { isValid: false, error: 'LED refresh rate must be between 10 and 120 Hz' };
    }
    
    // Effect-specific validation
    const effects = ledConfig.effects;
    
    // Chase Single validation
    if (effects.chaseSingle.speed < 1 || effects.chaseSingle.speed > 100) {
        return { isValid: false, error: 'Chase Single speed must be between 1 and 100' };
    }
    
    // Chase Multi validation
    if (effects.chaseMulti.speed < 1 || effects.chaseMulti.speed > 100) {
        return { isValid: false, error: 'Chase Multi speed must be between 1 and 100' };
    }
    
    // Matrix validation
    if (effects.matrix.drops < 1 || effects.matrix.drops > 20) {
        return { isValid: false, error: 'Matrix drops must be between 1 and 20' };
    }
    
    // Twinkle validation
    if (effects.twinkle.density < 1 || effects.twinkle.density > 20) {
        return { isValid: false, error: 'Twinkle density must be between 1 and 20' };
    }
    
    if (effects.twinkle.fadeSpeed < 1 || effects.twinkle.fadeSpeed > 255) {
        return { isValid: false, error: 'Twinkle fade speed must be between 1 and 255' };
    }
    
    return { isValid: true };
}

// Make functions available globally
window.LEDConfig = {
    populateLedForm,
    collectLedFormData,
    validateLedConfig,
    colorToHtml,
    colorToRgba
};
