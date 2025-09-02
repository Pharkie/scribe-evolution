#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

// Find the .claude directory by traversing up from multiple starting points
function findClaudeDir() {
  const searchPaths = [
    // Script's directory
    __dirname,
    // Current working directory
    process.cwd(),
    // Environment variable if set
    process.env.CLAUDE_DIR,
    // Common project root patterns
    path.resolve(process.cwd(), ".."),
    path.resolve(process.cwd(), "../.."),
    // If we're in a nested structure, try going up several levels
    path.resolve(__dirname, ".."),
    path.resolve(__dirname, "../.."),
    path.resolve(__dirname, "../../.."),
  ].filter(Boolean); // Remove undefined values

  // First, check if we're already in a .claude directory structure
  for (const startPath of searchPaths) {
    let currentDir = startPath;

    // Check if current path contains .claude
    if (currentDir.includes(".claude")) {
      const parts = currentDir.split(path.sep);
      const claudeIndex = parts.findIndex((part) => part === ".claude");
      if (claudeIndex !== -1) {
        const claudeDir = parts.slice(0, claudeIndex + 1).join(path.sep);
        if (fs.existsSync(claudeDir)) {
          return claudeDir;
        }
      }
    }

    // Traverse up from each starting path
    while (currentDir && currentDir !== path.dirname(currentDir)) {
      const claudeDir = path.join(currentDir, ".claude");
      if (fs.existsSync(claudeDir) && fs.statSync(claudeDir).isDirectory()) {
        return claudeDir;
      }
      currentDir = path.dirname(currentDir);
    }
  }

  // Last resort: check common locations
  const commonLocations = [
    path.join(process.env.HOME || process.env.USERPROFILE || "", ".claude"),
    "/tmp/.claude",
    path.join(process.cwd(), ".claude"),
  ].filter(Boolean);

  for (const location of commonLocations) {
    if (fs.existsSync(location) && fs.statSync(location).isDirectory()) {
      return location;
    }
  }

  // If all else fails, create debug info
  console.error("Debug info:");
  console.error("__dirname:", __dirname);
  console.error("process.cwd():", process.cwd());
  console.error("Searched paths:", searchPaths);

  throw new Error("Could not find .claude directory");
}

try {
  const claudeDir = findClaudeDir();
  const awFilePath = path.join(claudeDir, "commands", "aw.md");

  if (fs.existsSync(awFilePath)) {
    const content = fs.readFileSync(awFilePath, "utf8");
    console.log(content);
  } else {
    console.error(`Warning: ${awFilePath} not found`);
  }
} catch (error) {
  console.error(`Error: ${error.message}`);
  process.exit(1);
}
