/**
 * Mock Server Authentication Module
 * Simulates the ESP32 session-based authentication system
 */

const crypto = require("crypto");

// Session storage (in-memory for mock server)
const sessions = new Map();

// Configuration matching ESP32 settings
const SESSION_COOKIE_NAME = "scribe_session";
const SESSION_TIMEOUT_MS = 14400000; // 4 hours
const SESSION_TOKEN_LENGTH = 32;
const SESSION_COOKIE_OPTIONS = "HttpOnly; SameSite=Strict; Path=/";

// Statistics
let totalSessionsCreated = 0;

/**
 * Generate a secure session token
 */
function generateSecureToken() {
  return crypto.randomBytes(SESSION_TOKEN_LENGTH / 2).toString("hex");
}

/**
 * Create a new session
 */
function createSession(clientIP = "127.0.0.1") {
  const token = generateSecureToken();
  const session = {
    token,
    clientIP,
    lastActivity: Date.now(),
    active: true,
  };

  sessions.set(token, session);
  totalSessionsCreated++;

  console.log(
    `[AUTH] Created session for IP ${clientIP} (token: ${token.substring(0, 8)}...)`,
  );
  return token;
}

/**
 * Validate a session token
 */
function validateSession(token, clientIP = "127.0.0.1") {
  if (!token || token.length !== SESSION_TOKEN_LENGTH) {
    return false;
  }

  const session = sessions.get(token);
  if (!session || !session.active) {
    return false;
  }

  // Check if session has expired
  const currentTime = Date.now();
  if (currentTime - session.lastActivity > SESSION_TIMEOUT_MS) {
    session.active = false;
    sessions.delete(token);
    console.log(`[AUTH] Session expired for IP ${session.clientIP}`);
    return false;
  }

  // Validate IP (relaxed for mock server - just log differences)
  if (session.clientIP !== clientIP) {
    console.log(
      `[AUTH] Warning: Session IP mismatch. Expected: ${session.clientIP}, Got: ${clientIP}`,
    );
  }

  return true;
}

/**
 * Refresh a session's activity timestamp
 */
function refreshSession(token) {
  const session = sessions.get(token);
  if (session && session.active) {
    session.lastActivity = Date.now();
    console.log(`[AUTH] Refreshed session for IP ${session.clientIP}`);
  }
}

/**
 * Check if a path requires authentication
 */
function requiresAuthentication(path, isAPMode = false) {
  // No auth required in AP mode
  if (isAPMode) {
    return false;
  }

  // Public paths
  const publicPaths = [
    "/",
    "/index.html",
    "/setup.html",
    "/404.html",
    "/favicon.svg",
    "/css/",
    "/js/",
    "/images/",
    "/api/routes",
    "/ncsi.txt",
  ];

  for (const publicPath of publicPaths) {
    if (
      path === publicPath ||
      (publicPath !== "/" && path.startsWith(publicPath))
    ) {
      return false;
    }
  }

  // Setup endpoints are public
  if (
    path.startsWith("/api/setup") ||
    path.startsWith("/api/wifi-scan") ||
    path.startsWith("/api/test-wifi") ||
    path.startsWith("/api/timezones")
  ) {
    return false;
  }

  // All other API endpoints require authentication
  if (path.startsWith("/api/")) {
    return true;
  }

  // Default: no authentication required for static assets
  return false;
}

/**
 * Extract session token from request cookies
 */
function getSessionToken(req) {
  const cookieHeader = req.headers.cookie;
  if (!cookieHeader) {
    return null;
  }

  const cookies = cookieHeader.split(";").map((c) => c.trim());
  const sessionCookie = cookies.find((c) =>
    c.startsWith(`${SESSION_COOKIE_NAME}=`),
  );

  if (!sessionCookie) {
    return null;
  }

  const token = sessionCookie.split("=")[1];
  return token && token.length === SESSION_TOKEN_LENGTH ? token : null;
}

/**
 * Create session cookie header value
 */
function getSessionCookieValue(sessionToken) {
  const maxAge = Math.floor(SESSION_TIMEOUT_MS / 1000);
  return `${SESSION_COOKIE_NAME}=${sessionToken}; ${SESSION_COOKIE_OPTIONS}; Max-Age=${maxAge}`;
}

/**
 * Authentication middleware for mock server
 */
function authenticatedHandler(req, res, handler, ctx) {
  const path = new URL(req.url, `http://${req.headers.host}`).pathname;

  // Check if authentication is required
  if (!requiresAuthentication(path, ctx?.isAPMode?.())) {
    return handler(req, res);
  }

  // Extract session token
  const sessionToken = getSessionToken(req);
  const clientIP =
    req.connection?.remoteAddress || req.socket?.remoteAddress || "127.0.0.1";

  // Validate session
  if (sessionToken && validateSession(sessionToken, clientIP)) {
    // Refresh session activity
    refreshSession(sessionToken);
    return handler(req, res);
  } else {
    // Unauthorized - return 401
    console.log(
      `[AUTH] Unauthorized access attempt from ${clientIP} to ${path}`,
    );
    res.writeHead(401, { "Content-Type": "application/json" });
    res.end(JSON.stringify({ error: "Authentication required", code: 401 }));
  }
}

/**
 * Handle session creation on index page access
 */
function handleSessionCreation(req, res, next) {
  const path = new URL(req.url, `http://${req.headers.host}`).pathname;

  // Create session on access to root or index.html
  if (path === "/" || path === "/index.html") {
    const existingToken = getSessionToken(req);
    const clientIP =
      req.connection?.remoteAddress || req.socket?.remoteAddress || "127.0.0.1";

    // Only create new session if current one is invalid
    if (!existingToken || !validateSession(existingToken, clientIP)) {
      const newToken = createSession(clientIP);
      const cookieValue = getSessionCookieValue(newToken);

      // Set cookie in response (will be handled by the static file handler)
      res.mockSessionCookie = cookieValue;
    }
  }

  return next();
}

/**
 * Get authentication statistics
 */
function getAuthStats() {
  const activeSessionCount = Array.from(sessions.values()).filter(
    (s) => s.active,
  ).length;
  return {
    activeSessionCount,
    totalSessionsCreated,
  };
}

/**
 * Clean up expired sessions
 */
function cleanupExpiredSessions() {
  const currentTime = Date.now();
  let cleanedUp = 0;

  for (const [token, session] of sessions.entries()) {
    if (currentTime - session.lastActivity > SESSION_TIMEOUT_MS) {
      sessions.delete(token);
      cleanedUp++;
    }
  }

  if (cleanedUp > 0) {
    console.log(`[AUTH] Cleaned up ${cleanedUp} expired sessions`);
  }
}

// Clean up sessions periodically
setInterval(cleanupExpiredSessions, 300000); // 5 minutes

module.exports = {
  createSession,
  validateSession,
  refreshSession,
  requiresAuthentication,
  getSessionToken,
  getSessionCookieValue,
  authenticatedHandler,
  handleSessionCreation,
  getAuthStats,
  cleanupExpiredSessions,
  SESSION_COOKIE_NAME,
};
