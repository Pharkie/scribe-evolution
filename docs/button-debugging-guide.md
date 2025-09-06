# Button Debugging Guide

Quick checks to diagnose hardware button issues.

## Wiring & GPIO

- Verify each button’s GPIO matches Settings → Buttons.
- Avoid unsafe pins (flash/USB); see warnings in logs.
- Confirm pull mode: active‑low uses `INPUT_PULLUP` (pressed = LOW).

## Debounce & Long Press

- Debounce: `buttonDebounceMs` filters chatter.
- Long press threshold: `buttonLongPressMs`.
- If long presses don’t trigger, confirm press duration in logs.

## Rate Limiting

- Per‑button limits: min interval + max per minute.
- If presses are ignored, logs show “rate limited”.

## Async Task Guard

- Only one action runs at a time (mutex‑guarded task).
- If a second press is ignored, wait for prior action to finish.

## LED Effects

- If enabled, effects map per button (short/long).
- Check effect names; “none” disables.

## MQTT vs Local

- If a button has an MQTT topic, content is sent via MQTT only.
- Without topic, it prints locally.

## Logs

- Enable VERBOSE logs. Look for:
  - PRESSED/RELEASED lines with GPIO
  - SHORT/LONG press classification
  - Async task creation and completion
  - Rate limit decisions
