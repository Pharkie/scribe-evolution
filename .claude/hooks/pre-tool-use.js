#!/usr/bin/env node

const fs = require("fs");
const path = require("path");

// Find the .claude directory by traversing up from script location OR current working directory
function findClaudeDir() {
  // Try from script's directory first (when called with absolute path)
  let currentDir = __dirname;
  if (currentDir.endsWith(".claude/hooks")) {
    return path.dirname(currentDir);
  }

  // Try from current working directory (when called with relative path)
  currentDir = process.cwd();
  while (currentDir !== path.dirname(currentDir)) {
    const claudeDir = path.join(currentDir, ".claude");
    if (fs.existsSync(claudeDir)) {
      return claudeDir;
    }
    currentDir = path.dirname(currentDir);
  }

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
