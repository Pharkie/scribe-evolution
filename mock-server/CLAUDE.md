# Mock Server - CLAUDE.md

<system_context>
Node.js development server simulating ESP32 API endpoints.
Enables frontend development without hardware dependencies.
</system_context>

<critical_notes>

- Runs on port 3001 (avoid conflicts with other services)
- Simulates all ESP32 API endpoints with realistic responses
- Maintains state between requests for testing workflows
- Use for rapid frontend iteration
  </critical_notes>

<paved_path>
Development Workflow:

1. Start mock server: `npm start` (from mock-server/ directory)
2. Access at http://localhost:3001/
3. Make frontend changes in /src/data/
4. Test against mock API
5. When ready, build and deploy to ESP32
   </paved_path>

<patterns>
// Adding new endpoint
app.get('/api/new-endpoint', (req, res) => {
    res.json({
        status: 'ok',
        data: mockData.newEndpoint
    });
});

// POST endpoint with validation
app.post('/api/config', (req, res) => {
const { deviceOwner } = req.body;

    if (!deviceOwner || deviceOwner.trim().length === 0) {
        return res.status(400).json({
            error: 'Device owner cannot be empty'
        });
    }

    // Update mock state
    mockData.config.deviceOwner = deviceOwner;
    res.status(200).send(); // Empty success response

});

// Simulating ESP32 delays
app.get('/api/wifi/scan', async (req, res) => {
// Simulate scan time
await new Promise(resolve => setTimeout(resolve, 2000));
res.json(mockData.wifiNetworks);
});
</patterns>

<common_tasks>
Adding mock for new feature:

1. Add endpoint to mock-server/mock-api.js
2. Add realistic mock data
3. Match ESP32 response format exactly
4. Test error cases and success flows
   </common_tasks>

<hatch>
Rapid Iteration:
- No ESP32 upload cycles
- Instant changes with nodemon
- Easy debugging with console logs
- Test edge cases without hardware
</hatch>

<fatal_implications>

- Mock responses don't match ESP32 = Frontend breaks on real hardware
- Missing error cases = Untested failure paths
- Wrong response format = Integration issues
  </fatal_implications>
