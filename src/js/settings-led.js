/**
 * @file settings-led.js
 * @brief LED functionality module - handles all LED-related operations
 * @description Focused module for LED configuration, effects, and testing
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
    if (!config.leds) {
        return;
    }

    // Hardware settings
    const ledPin = document.getElementById('led-pin');
    const ledCount = document.getElementById('led-count');
    const ledBrightness = document.getElementById('led-brightness');
    const ledRefreshRate = document.getElementById('led-refresh-rate');
    
    if (ledPin) ledPin.value = config.leds.pin || 4;
    if (ledCount) ledCount.value = config.leds.count || 30;
    if (ledBrightness) ledBrightness.value = config.leds.brightness || 64;
    if (ledRefreshRate) ledRefreshRate.value = config.leds.refreshRate || 60;
    
    // Populate autonomous per-effect configurations
    const effects = config.leds.effects || {};
    
    // Chase Single
    const chaseSingle = effects.chaseSingle || {};
    
    const chaseSingleElements = {
        speed: document.getElementById('chase-single-speed'),
        trailLength: document.getElementById('chase-single-trail-length'),
        trailFade: document.getElementById('chase-single-trail-fade'),
        color: document.getElementById('chase-single-color')
    };
    
    if (chaseSingleElements.speed) chaseSingleElements.speed.value = chaseSingle.speed || 5;
    if (chaseSingleElements.trailLength) chaseSingleElements.trailLength.value = chaseSingle.trailLength || 15;
    if (chaseSingleElements.trailFade) chaseSingleElements.trailFade.value = chaseSingle.trailFade || 15;
    if (chaseSingleElements.color) chaseSingleElements.color.value = colorToHtml(chaseSingle.defaultColor, '#0062ff');

    // Chase Multi
    const chaseMulti = effects.chaseMulti || {};
    
    const chaseMultiElements = {
        speed: document.getElementById('chase-multi-speed'),
        trailLength: document.getElementById('chase-multi-trail-length'),
        trailFade: document.getElementById('chase-multi-trail-fade'),
        colorSpacing: document.getElementById('chase-multi-color-spacing'),
        color1: document.getElementById('chase-multi-color1'),
        color2: document.getElementById('chase-multi-color2'),
        color3: document.getElementById('chase-multi-color3')
    };
    
    if (chaseMultiElements.speed) chaseMultiElements.speed.value = chaseMulti.speed || 2;
    if (chaseMultiElements.trailLength) chaseMultiElements.trailLength.value = chaseMulti.trailLength || 20;
    if (chaseMultiElements.trailFade) chaseMultiElements.trailFade.value = chaseMulti.trailFade || 20;
    if (chaseMultiElements.colorSpacing) chaseMultiElements.colorSpacing.value = chaseMulti.colorSpacing || 12;
    if (chaseMultiElements.color1) chaseMultiElements.color1.value = colorToHtml(chaseMulti.color1, '#ff9900');
    if (chaseMultiElements.color2) chaseMultiElements.color2.value = colorToHtml(chaseMulti.color2, '#008f00');
    if (chaseMultiElements.color3) chaseMultiElements.color3.value = colorToHtml(chaseMulti.color3, '#78cffe');

    // Matrix
    const matrix = effects.matrix || {};
    
    const matrixElements = {
        speed: document.getElementById('matrix-speed'),
        drops: document.getElementById('matrix-drops'),
        backgroundFade: document.getElementById('matrix-background-fade'),
        trailFade: document.getElementById('matrix-trail-fade'),
        brightnessFade: document.getElementById('matrix-brightness-fade'),
        color: document.getElementById('matrix-color')
    };
    
    if (matrixElements.speed) matrixElements.speed.value = matrix.speed || 3;
    if (matrixElements.drops) matrixElements.drops.value = matrix.drops || 5;
    if (matrixElements.backgroundFade) matrixElements.backgroundFade.value = matrix.backgroundFade || 64;
    if (matrixElements.trailFade) matrixElements.trailFade.value = matrix.trailFade || 32;
    if (matrixElements.brightnessFade) matrixElements.brightnessFade.value = matrix.brightnessFade || 40;
    if (matrixElements.color) matrixElements.color.value = colorToHtml(matrix.defaultColor, '#009100');

    // Twinkle
    const twinkle = effects.twinkle || {};
    
    const twinkleElements = {
        density: document.getElementById('twinkle-density'),
        fadeSpeed: document.getElementById('twinkle-fade-speed'),
        minBrightness: document.getElementById('twinkle-min-brightness'),
        maxBrightness: document.getElementById('twinkle-max-brightness'),
        color: document.getElementById('twinkle-color')
    };
    
    if (twinkleElements.density) twinkleElements.density.value = twinkle.density || 8;
    if (twinkleElements.fadeSpeed) twinkleElements.fadeSpeed.value = twinkle.fadeSpeed || 5;
    if (twinkleElements.minBrightness) twinkleElements.minBrightness.value = twinkle.minBrightness || 50;
    if (twinkleElements.maxBrightness) twinkleElements.maxBrightness.value = twinkle.maxBrightness || 255;
    if (twinkleElements.color) twinkleElements.color.value = colorToHtml(twinkle.defaultColor, '#ffffff');

    // Pulse
    const pulse = effects.pulse || {};
    
    const pulseElements = {
        speed: document.getElementById('pulse-speed'),
        minBrightness: document.getElementById('pulse-min-brightness'),
        maxBrightness: document.getElementById('pulse-max-brightness'),
        waveFrequency: document.getElementById('pulse-wave-frequency'),
        color: document.getElementById('pulse-color')
    };
    
    if (pulseElements.speed) pulseElements.speed.value = pulse.speed || 4;
    if (pulseElements.minBrightness) pulseElements.minBrightness.value = pulse.minBrightness || 0;
    if (pulseElements.maxBrightness) pulseElements.maxBrightness.value = pulse.maxBrightness || 255;
    if (pulseElements.waveFrequency) pulseElements.waveFrequency.value = pulse.waveFrequency || 0.05;
    if (pulseElements.color) pulseElements.color.value = colorToHtml(pulse.defaultColor, '#ff00f2');

    // Rainbow
    const rainbow = effects.rainbow || {};
    
    const rainbowElements = {
        speed: document.getElementById('rainbow-speed'),
        saturation: document.getElementById('rainbow-saturation'),
        brightness: document.getElementById('rainbow-brightness'),
        hueStep: document.getElementById('rainbow-hue-step')
    };
    
    if (rainbowElements.speed) rainbowElements.speed.value = rainbow.speed || 2.0;
    if (rainbowElements.saturation) rainbowElements.saturation.value = rainbow.saturation || 255;
    if (rainbowElements.brightness) rainbowElements.brightness.value = rainbow.brightness || 255;
    if (rainbowElements.hueStep) rainbowElements.hueStep.value = rainbow.hueStep || 2.5;
}

/**
 * Collect basic LED hardware configuration (no effects)
 * @returns {Object} Basic LED configuration object
 */
function collectBasicLedConfig() {
    // Helper function to safely get integer value with fallback
    const getIntValue = (id, fallback) => {
        const element = document.getElementById(id);
        if (!element) return fallback;
        const value = parseInt(element.value);
        return isNaN(value) ? fallback : value;
    };

    return {
        pin: getIntValue('led-pin', 4),
        count: getIntValue('led-count', 30),
        brightness: getIntValue('led-brightness', 64),
        refreshRate: getIntValue('led-refresh-rate', 60)
        // Note: No effects configuration - that's handled by demo buttons
    };
}

/**
 * Demo/test a specific LED effect with current form values
 * @param {string} effectName - Name of the effect to demo
 */
async function demoLedEffect(effectName) {
    try {
        // Helper functions to get current form values
        const getIntValue = (id, fallback) => {
            const element = document.getElementById(id);
            if (!element) return fallback;
            const value = parseInt(element.value);
            return isNaN(value) ? fallback : value;
        };

        const getFloatValue = (id, fallback) => {
            const element = document.getElementById(id);
            if (!element) return fallback;
            const value = parseFloat(element.value);
            return isNaN(value) ? fallback : value;
        };

        const getStringValue = (id, fallback) => {
            const element = document.getElementById(id);
            return element ? element.value || fallback : fallback;
        };

        // Build effect configuration based on effect type
        let effectConfig = {
            effect: effectName,
            duration: 10000 // 10 seconds demo
        };

        // Add effect-specific parameters
        switch (effectName) {
            case 'chase_single':
                effectConfig.speed = getIntValue('chase-single-speed', 5);
                effectConfig.trailLength = getIntValue('chase-single-trail-length', 15);
                effectConfig.trailFade = getIntValue('chase-single-trail-fade', 15);
                effectConfig.color1 = getStringValue('chase-single-color', '#0062ff');
                break;
            case 'chase_multi':
                effectConfig.speed = getIntValue('chase-multi-speed', 2);
                effectConfig.trailLength = getIntValue('chase-multi-trail-length', 20);
                effectConfig.trailFade = getIntValue('chase-multi-trail-fade', 20);
                effectConfig.colorSpacing = getIntValue('chase-multi-color-spacing', 12);
                effectConfig.color1 = getStringValue('chase-multi-color1', '#ff9900');
                effectConfig.color2 = getStringValue('chase-multi-color2', '#008f00');
                effectConfig.color3 = getStringValue('chase-multi-color3', '#78cffe');
                break;
            case 'matrix':
                effectConfig.speed = getIntValue('matrix-speed', 3);
                effectConfig.drops = getIntValue('matrix-drops', 5);
                effectConfig.backgroundFade = getIntValue('matrix-background-fade', 64);
                effectConfig.trailFade = getIntValue('matrix-trail-fade', 32);
                effectConfig.brightnessFade = getIntValue('matrix-brightness-fade', 40);
                effectConfig.color1 = getStringValue('matrix-color', '#009100');
                break;
            case 'twinkle':
                effectConfig.density = getIntValue('twinkle-density', 8);
                effectConfig.fadeSpeed = getIntValue('twinkle-fade-speed', 5);
                effectConfig.minBrightness = getIntValue('twinkle-min-brightness', 50);
                effectConfig.maxBrightness = getIntValue('twinkle-max-brightness', 255);
                effectConfig.color1 = getStringValue('twinkle-color', '#ffffff');
                break;
            case 'pulse':
                effectConfig.speed = getIntValue('pulse-speed', 4);
                effectConfig.minBrightness = getIntValue('pulse-min-brightness', 0);
                effectConfig.maxBrightness = getIntValue('pulse-max-brightness', 255);
                effectConfig.waveFrequency = getFloatValue('pulse-wave-frequency', 0.05);
                effectConfig.color1 = getStringValue('pulse-color', '#ff00f2');
                break;
            case 'rainbow':
                effectConfig.speed = getFloatValue('rainbow-speed', 2.0);
                effectConfig.saturation = getIntValue('rainbow-saturation', 255);
                effectConfig.brightness = getIntValue('rainbow-brightness', 255);
                effectConfig.hueStep = getFloatValue('rainbow-hue-step', 2.5);
                break;
        }

        console.log(`Demoing ${effectName} with config:`, effectConfig);

        const response = await fetch('/api/led-effect', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            },
            body: JSON.stringify(effectConfig)
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        const result = await response.json();
        
        if (window.SettingsCore && window.SettingsCore.showMessage) {
            window.SettingsCore.showMessage(`${effectName} demo started!`, 'success');
        }
        
    } catch (error) {
        console.error(`Failed to demo LED effect ${effectName}:`, error);
        if (window.SettingsCore && window.SettingsCore.showMessage) {
            window.SettingsCore.showMessage(`Failed to demo ${effectName}: ${error.message}`, 'error');
        }
    }
}

/**
 * Turn off LEDs (same as before)
 */
async function turnOffLeds() {
    try {
        const response = await fetch('/api/leds-off', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
            }
        });

        if (!response.ok) {
            throw new Error(`HTTP error! status: ${response.status}`);
        }

        if (window.SettingsCore && window.SettingsCore.showMessage) {
            window.SettingsCore.showMessage('LEDs turned off successfully!', 'success');
        }
        
    } catch (error) {
        console.error('Failed to turn off LEDs:', error);
        if (window.SettingsCore && window.SettingsCore.showMessage) {
            window.SettingsCore.showMessage(`Failed to turn off LEDs: ${error.message}`, 'error');
        }
    }
}

/**
 * Validate basic LED configuration data
 * @param {Object} ledConfig - LED configuration object
 * @returns {Object} Validation result with isValid boolean and error message
 */
function validateLedConfig(ledConfig) {
    if (!ledConfig) {
        return { isValid: false, error: 'LED configuration is missing' };
    }

    // Hardware validation only
    if (ledConfig.pin < 0 || ledConfig.pin > 21) {
        return { isValid: false, error: 'LED pin must be between 0 and 21 (ESP32-C3 compatible)' };
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
    
    return { isValid: true };
}

// Export LED module
window.SettingsLED = {
    populateLedForm,
    collectLedConfig: collectBasicLedConfig, // Alias for consistency
    validateLedConfig,
    colorToHtml,
    colorToRgba,
    demoLedEffect
};
