const esbuild = require("esbuild");
const fs = require("fs");
const path = require("path");

// esbuild plugins for proper bundling
const multiEntryPlugin = {
  name: "multi-entry",
  setup(build) {
    build.onResolve({ filter: /^multi-entry:/ }, (args) => {
      const files = args.path.replace("multi-entry:", "").split(",");
      return {
        path: args.path,
        namespace: "multi-entry",
        pluginData: { files },
      };
    });

    build.onLoad({ filter: /.*/, namespace: "multi-entry" }, (args) => {
      const files = args.pluginData.files;
      const contents = files
        .map((file, index) => {
          const filePath = file.trim();
          // Check if file exists and read it directly
          if (fs.existsSync(filePath)) {
            return fs.readFileSync(filePath, "utf8");
          } else {
            console.warn(`Warning: File not found: ${filePath}`);
            return `// File not found: ${filePath}`;
          }
        })
        .join("\n\n");

      return {
        contents,
        loader: "js",
      };
    });
  },
};

// Enhanced bundling function
async function buildWithEsbuild(config) {
  const { input, output, minify = false } = config;

  console.log(`🚀 Bundling: ${input.join(", ")} -> ${output}`);

  try {
    let entryPoint;
    if (input.length === 1) {
      entryPoint = input[0];
    } else {
      // Use virtual multi-entry for multiple files
      entryPoint = `multi-entry:${input.join(",")}`;
    }

    const result = await esbuild.build({
      entryPoints: [entryPoint],
      bundle: true,
      minify: minify,
      treeShaking: true,
      target: "es2017",
      format: "iife",
      platform: "browser",
      outfile: output,
      sourcemap: false,
      plugins: input.length > 1 ? [multiEntryPlugin] : [], // Only use multi-entry for legacy concatenation
      // Node modules resolution
      resolveExtensions: [".js", ".jsx", ".ts", ".tsx"],
      // Optimize for browser
      legalComments: "none",
      // Bundle all dependencies for Alpine.js builds
      external: [],
      // Better minification
      keepNames: false,
      write: true,
      logLevel: "silent",
    });

    const stats = fs.statSync(output);
    console.log(`✅ ${output} (${stats.size} bytes)`);

    return result;
  } catch (error) {
    console.error(`❌ Failed to bundle ${output}:`, error.message);
    throw error;
  }
}

// Build configurations matching current package.json structure
const buildConfigs = {
  // Alpine.js library with Collapse plugin
  alpine: {
    input: ["src/data/js/alpine-with-collapse.js"],
    output: "data/js/alpine.js",
    minify: false,
  },

  alpineProd: {
    input: ["src/data/js/alpine-with-collapse.js"],
    output: "data/js/alpine.js",
    minify: true,
  },

  // Common app bundle
  common: {
    input: ["src/data/js/icons.js", "src/data/js/app-common.js"],
    output: "data/js/app-common.js",
    minify: false,
  },

  commonProd: {
    input: ["src/data/js/icons.js", "src/data/js/app-common.js"],
    output: "data/js/app-common.js",
    minify: true,
  },

  // Page-specific bundles
  index: {
    input: ["src/data/js/pages/index.js"], // Single ES6 module entry point
    output: "data/js/page-index.js",
    minify: false,
  },

  indexProd: {
    input: ["src/data/js/pages/index.js"], // Single ES6 module entry point
    output: "data/js/page-index.js",
    minify: true,
  },

  setup: {
    input: ["src/data/js/pages/setup.js"], // Single ES6 module entry point
    output: "data/js/page-setup.js",
    minify: false,
  },

  setupProd: {
    input: ["src/data/js/pages/setup.js"], // Single ES6 module entry point
    output: "data/js/page-setup.js",
    minify: true,
  },

  404: {
    input: ["src/data/js/pages/404.js"], // Single ES6 module entry point
    output: "data/js/page-404.js",
    minify: false,
  },

  "404Prod": {
    input: ["src/data/js/pages/404.js"], // Single ES6 module entry point
    output: "data/js/page-404.js",
    minify: true,
  },

  // Device settings page
  device: {
    input: ["src/data/js/pages/settings-device.js"], // Single ES6 module entry point
    output: "data/js/page-settings-device.js",
    minify: false,
  },

  deviceProd: {
    input: ["src/data/js/pages/settings-device.js"], // Single ES6 module entry point
    output: "data/js/page-settings-device.js",
    minify: true,
  },

  // WiFi settings page
  wifi: {
    input: ["src/data/js/pages/settings-wifi.js"], // Single ES6 module entry point
    output: "data/js/page-settings-wifi.js",
    minify: false,
  },

  wifiProd: {
    input: ["src/data/js/pages/settings-wifi.js"], // Single ES6 module entry point
    output: "data/js/page-settings-wifi.js",
    minify: true,
  },

  // MQTT settings page
  mqtt: {
    input: ["src/data/js/pages/settings-mqtt.js"], // Single ES6 module entry point
    output: "data/js/page-settings-mqtt.js",
    minify: false,
  },

  mqttProd: {
    input: ["src/data/js/pages/settings-mqtt.js"], // Single ES6 module entry point
    output: "data/js/page-settings-mqtt.js",
    minify: true,
  },

  // Settings overview page
  overview: {
    input: ["src/data/js/pages/settings-overview.js"], // Single ES6 module entry point
    output: "data/js/page-settings-overview.js",
    minify: false,
  },

  overviewProd: {
    input: ["src/data/js/pages/settings-overview.js"], // Single ES6 module entry point
    output: "data/js/page-settings-overview.js",
    minify: true,
  },

  // Memos settings page
  memos: {
    input: ["src/data/js/pages/settings-memos.js"], // Single ES6 module entry point
    output: "data/js/page-settings-memos.js",
    minify: false,
  },

  memosProd: {
    input: ["src/data/js/pages/settings-memos.js"], // Single ES6 module entry point
    output: "data/js/page-settings-memos.js",
    minify: true,
  },

  // Buttons settings page
  buttons: {
    input: ["src/data/js/pages/settings-buttons.js"], // Single ES6 module entry point
    output: "data/js/page-settings-buttons.js",
    minify: false,
  },

  buttonsProd: {
    input: ["src/data/js/pages/settings-buttons.js"], // Single ES6 module entry point
    output: "data/js/page-settings-buttons.js",
    minify: true,
  },

  leds: {
    input: ["src/data/js/pages/settings-leds.js"], // Single ES6 module entry point
    output: "data/js/page-settings-leds.js",
    minify: false,
  },

  ledsProd: {
    input: ["src/data/js/pages/settings-leds.js"], // Single ES6 module entry point
    output: "data/js/page-settings-leds.js",
    minify: true,
  },

  // Unbidden Ink settings page
  "unbidden-ink": {
    input: ["src/data/js/pages/settings-unbidden-ink.js"], // Single ES6 module entry point
    output: "data/js/page-settings-unbidden-ink.js",
    minify: false,
  },

  "unbidden-inkProd": {
    input: ["src/data/js/pages/settings-unbidden-ink.js"], // Single ES6 module entry point
    output: "data/js/page-settings-unbidden-ink.js",
    minify: true,
  },

  // Diagnostics overview page
  "diagnostics-overview": {
    input: ["src/data/js/pages/diagnostics-overview.js"], // Single ES6 module entry point
    output: "data/js/page-diagnostics-overview.js",
    minify: false,
  },

  "diagnostics-overviewProd": {
    input: ["src/data/js/pages/diagnostics-overview.js"], // Single ES6 module entry point
    output: "data/js/page-diagnostics-overview.js",
    minify: true,
  },

  // Diagnostics microcontroller page
  "diagnostics-microcontroller": {
    input: ["src/data/js/pages/diagnostics-microcontroller.js"], // Single ES6 module entry point
    output: "data/js/page-diagnostics-microcontroller.js",
    minify: false,
  },

  "diagnostics-microcontrollerProd": {
    input: ["src/data/js/pages/diagnostics-microcontroller.js"], // Single ES6 module entry point
    output: "data/js/page-diagnostics-microcontroller.js",
    minify: true,
  },

  // Diagnostics routes page
  "diagnostics-routes": {
    input: ["src/data/js/pages/diagnostics-routes.js"], // Single ES6 module entry point
    output: "data/js/page-diagnostics-routes.js",
    minify: false,
  },

  "diagnostics-routesProd": {
    input: ["src/data/js/pages/diagnostics-routes.js"], // Single ES6 module entry point
    output: "data/js/page-diagnostics-routes.js",
    minify: true,
  },

  // Diagnostics runtime config page
  "diagnostics-runtime-config": {
    input: ["src/data/js/pages/diagnostics-runtime-config.js"], // Single ES6 module entry point
    output: "data/js/page-diagnostics-runtime-config.js",
    minify: false,
  },

  "diagnostics-runtime-configProd": {
    input: ["src/data/js/pages/diagnostics-runtime-config.js"], // Single ES6 module entry point
    output: "data/js/page-diagnostics-runtime-config.js",
    minify: true,
  },

  // Diagnostics NVS page
  "diagnostics-nvs": {
    input: ["src/data/js/pages/diagnostics-nvs.js"], // Single ES6 module entry point
    output: "data/js/page-diagnostics-nvs.js",
    minify: false,
  },

  "diagnostics-nvsProd": {
    input: ["src/data/js/pages/diagnostics-nvs.js"], // Single ES6 module entry point
    output: "data/js/page-diagnostics-nvs.js",
    minify: true,
  },
};

// Build functions
async function build(configName) {
  const config = buildConfigs[configName];
  if (!config) {
    console.error(`Unknown build config: ${configName}`);
    process.exit(1);
  }

  try {
    console.log(`Building ${configName}...`);

    // Build all configs with esbuild now (including Alpine with plugins)
    await buildWithEsbuild(config);
  } catch (error) {
    console.error(`❌ Error building ${configName}:`, error);
    process.exit(1);
  }
}

async function buildAll(production = false) {
  const suffix = production ? "Prod" : "";
  const configs = [
    `alpine${suffix}`,
    `common${suffix}`,
    `index${suffix}`,
    `setup${suffix}`,
    `404${suffix}`,
    `device${suffix}`,
    `wifi${suffix}`,
    `mqtt${suffix}`,
    `overview${suffix}`,
    `memos${suffix}`,
    `buttons${suffix}`,
    `leds${suffix}`,
    `unbidden-ink${suffix}`,
    `diagnostics-overview${suffix}`,
    `diagnostics-microcontroller${suffix}`,
    `diagnostics-routes${suffix}`,
    `diagnostics-runtime-config${suffix}`,
    `diagnostics-nvs${suffix}`,
  ];

  console.log(`Building all configs (production: ${production})...`);

  for (const config of configs) {
    await build(config);
  }

  console.log("🎉 All builds completed successfully!");
}

// Command line interface
if (require.main === module) {
  const args = process.argv.slice(2);
  const command = args[0];

  if (command === "all") {
    buildAll(false);
  } else if (command === "prod") {
    buildAll(true);
  } else if (command && buildConfigs[command]) {
    build(command);
  } else {
    console.log("Usage:");
    console.log("  node esbuild.config.js all     # Build all configs (dev)");
    console.log(
      "  node esbuild.config.js prod    # Build all configs (production)",
    );
    console.log("  node esbuild.config.js <name>  # Build specific config");
    console.log("");
    console.log("Available configs:", Object.keys(buildConfigs).join(", "));
  }
}

module.exports = { buildConfigs, build, buildAll };
