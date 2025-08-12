/**
 * @file fetch_helpers.h
 * @brief Common HTTP fetch patterns for JavaScript
 */

// Common fetch error handling pattern
function handleFetchResponse(response) {
    if (response.ok) {
        return response.json();
    }
    return response.json().then(data => {
        const errorMessage = data.error || data.message || `HTTP ${response.status}: ${response.statusText}`;
        throw new Error(errorMessage);
    }).catch(parseError => {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
    });
}

// Common fetch POST with JSON pattern
function fetchJSON(url, data) {
    return fetch(url, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    }).then(handleFetchResponse);
}

// Common two-step action pattern (generate content, then deliver)
function sendTwoStepAction(contentEndpoint, target, actionConfig) {
    triggerConfetti();
    
    return fetch(contentEndpoint, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ target })
    })
    .then(handleFetchResponse)
    .then(result => {
        if (!result.content) {
            throw new Error('No content received from server');
        }

        let deliveryEndpoint, payload;
        if (target === 'local-direct') {
            deliveryEndpoint = '/print-local';
            payload = { message: result.content };
        } else if (target === 'local-mqtt') {
            deliveryEndpoint = '/mqtt-send';
            payload = { topic: 'scribe/Pharkie/inbox', message: result.content };
        } else {
            deliveryEndpoint = '/mqtt-send';
            payload = { topic: target, message: result.content };
        }

        return fetchJSON(deliveryEndpoint, payload);
    })
    .then(result => {
        if (!result.success && result.status !== 'success') {
            throw new Error(result.error || result.message || 'Unknown error occurred');
        }
        showSuccessToast(`${actionConfig.name} scribed`);
    })
    .catch(error => {
        console.error(`Failed to send ${actionConfig.name.toLowerCase()}:`, error);
        alert(`Your Scribe stumbled on that ${actionConfig.name.toLowerCase()}. Want to try again?\n\nWhat happened: ${error.message}`);
    });
}
