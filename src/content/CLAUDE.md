# Content Generation - CLAUDE.md

<system_context>
Content generation system: jokes, riddles, AI integration.
Unbidden Ink scheduling with ChatGPT API integration.
</system_context>

<critical_notes>

- All autoprompts defined in config.h (Creative, Wisdom, Humor, DoctorWho)
- Default prompt is Creative (defaultUnbiddenInkPrompt)
- Time-window scheduling prevents collisions
- API client handles rate limiting and errors
  </critical_notes>

<paved_path>
Unbidden Ink System:

1. Autoprompts defined as constants in config.h
2. Scheduling uses time windows to avoid conflicts
3. ChatGPT API client in utils/ handles requests
4. Generated content stored in Message structure

Content Flow:
User request → Content generation → Message → Printer
</paved_path>

<patterns>
// Using autoprompt from config
String prompt = getUnbiddenInkPrompt(); // Returns current autoprompt

// Scheduling check
if (isInUnbiddenInkWindow() && !hasRecentUnbiddenInk()) {
generateUnbiddenInkContent();
}

// Content generation
Message generateJoke() {
Message msg;
msg.content = getRandomJoke();
msg.source = "jokes";
msg.timestamp = getCurrentTime();
return msg;
}

// API integration
bool success = chatgptClient.generateContent(prompt, responseBuffer, bufferSize);
if (!success) {
logger.log("ChatGPT request failed");
return false;
}
</patterns>

<common_tasks>
Adding new content type:

1. Define generation function following Message pattern
2. Add to content menu/routing
3. Test content fits printer constraints
4. Add to scheduling if automatic
   </common_tasks>

<fatal_implications>

- Hardcode prompts = No customization
- Missing collision detection = Spam printing
- No API error handling = Silent failures
- Wrong Message format = Printer errors
  </fatal_implications>
