const VALIDATION_TYPES = {
  STRING: "string",
  NON_EMPTY_STRING: "non_empty_string",
  IANA_TIMEZONE: "iana_timezone",
  GPIO: "gpio",
  RANGE_INT: "range_int",
  BOOLEAN: "boolean",
  ENUM_STRING: "enum_string",
};

// ESP32-C3 GPIO validation (mirrors device)
const VALID_GPIOS = [-1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 20, 21];
const SAFE_GPIOS = [-1, 2, 4, 5, 6, 7, 10, 20, 21];

function validateField(fieldPath, value, fieldDef) {
  switch (fieldDef.type) {
    case VALIDATION_TYPES.STRING:
      return { valid: typeof value === "string" };
    case VALIDATION_TYPES.NON_EMPTY_STRING:
      if (typeof value !== "string")
        return { valid: false, error: `${fieldPath} must be a string` };
      if (!value.length)
        return { valid: false, error: `${fieldPath} cannot be empty` };
      return { valid: true };
    case VALIDATION_TYPES.IANA_TIMEZONE: {
      if (typeof value !== "string" || !value.length)
        return {
          valid: false,
          error: `${fieldPath} must be a non-empty string`,
        };
      if (value.length > 50)
        return { valid: false, error: `${fieldPath} timezone name too long` };
      const prefixes = [
        "Africa/",
        "America/",
        "Antarctica/",
        "Asia/",
        "Atlantic/",
        "Australia/",
        "Europe/",
        "Indian/",
        "Pacific/",
        "Etc/",
      ];
      const ok =
        value === "UTC" ||
        value === "GMT" ||
        prefixes.some((p) => value.startsWith(p));
      if (
        !ok ||
        value.startsWith("/") ||
        value.endsWith("/") ||
        value.includes(" ")
      )
        return {
          valid: false,
          error: `${fieldPath} invalid IANA timezone format: ${value}`,
        };
      return { valid: true };
    }
    case VALIDATION_TYPES.GPIO:
      if (typeof value !== "number")
        return { valid: false, error: `${fieldPath} must be a number` };
      if (!VALID_GPIOS.includes(value))
        return {
          valid: false,
          error: `${fieldPath} invalid GPIO pin: ${value}`,
        };
      if (!SAFE_GPIOS.includes(value))
        return {
          valid: false,
          error: `${fieldPath} GPIO ${value} is not safe to use`,
        };
      return { valid: true };
    case VALIDATION_TYPES.RANGE_INT:
      if (typeof value !== "number")
        return { valid: false, error: `${fieldPath} must be a number` };
      if (value < fieldDef.min || value > fieldDef.max)
        return {
          valid: false,
          error: `${fieldPath} must be between ${fieldDef.min} and ${fieldDef.max}`,
        };
      return { valid: true };
    case VALIDATION_TYPES.BOOLEAN:
      return { valid: typeof value === "boolean" };
    case VALIDATION_TYPES.ENUM_STRING:
      if (typeof value !== "string")
        return { valid: false, error: `${fieldPath} must be a string` };
      return { valid: fieldDef.values.includes(value) };
    default:
      return {
        valid: false,
        error: `${fieldPath} unsupported type: ${fieldDef.type}`,
      };
  }
}

function flattenObject(obj, prefix = "") {
  const out = {};
  for (const key in obj) {
    if (!Object.prototype.hasOwnProperty.call(obj, key)) continue;
    const val = obj[key];
    if (val && typeof val === "object" && !Array.isArray(val)) {
      Object.assign(out, flattenObject(val, `${prefix}${key}.`));
    } else {
      out[`${prefix}${key}`] = val;
    }
  }
  return out;
}

function validateConfigFields(configUpdate) {
  const flat = flattenObject(configUpdate);
  const T = VALIDATION_TYPES;
  const rules = {
    "device.owner": { type: T.NON_EMPTY_STRING },
    "device.timezone": { type: T.IANA_TIMEZONE },
    "device.maxCharacters": { type: T.RANGE_INT, min: 100, max: 10000 },
    "device.printerTxPin": { type: T.GPIO },
    "wifi.ssid": { type: T.NON_EMPTY_STRING },
    "wifi.password": { type: T.STRING },
    "mqtt.enabled": { type: T.BOOLEAN },
    "mqtt.server": { type: T.STRING },
    "mqtt.port": { type: T.RANGE_INT, min: 1, max: 65535 },
    "mqtt.username": { type: T.STRING },
    "mqtt.password": { type: T.STRING },
    "unbiddenInk.enabled": { type: T.BOOLEAN },
    "unbiddenInk.startHour": { type: T.RANGE_INT, min: 0, max: 24 },
    "unbiddenInk.endHour": { type: T.RANGE_INT, min: 0, max: 24 },
    "unbiddenInk.frequencyMinutes": { type: T.RANGE_INT, min: 1, max: 1440 },
  };
  for (const k in flat) {
    if (!rules[k]) continue;
    const r = validateField(k, flat[k], rules[k]);
    if (!r.valid) return r;
  }
  return { valid: true };
}

function logProcessedFields(configUpdate) {
  const flat = flattenObject(configUpdate);
  const keys = Object.keys(flat);
  console.log(`✅ Processed ${keys.length} fields:`);
  keys.forEach((k) => console.log(`   • ${k}: ${JSON.stringify(flat[k])}`));
}

module.exports = {
  VALIDATION_TYPES,
  validateField,
  flattenObject,
  validateConfigFields,
  logProcessedFields,
};
