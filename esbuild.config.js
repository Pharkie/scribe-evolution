const esbuild = require('esbuild');
const fs = require('fs');
const path = require('path');

// esbuild plugins for proper bundling
const multiEntryPlugin = {
  name: 'multi-entry',
  setup(build) {
    build.onResolve({ filter: /^multi-entry:/ }, (args) => {
      const files = args.path.replace('multi-entry:', '').split(',');
      return {
        path: args.path,
        namespace: 'multi-entry',
        pluginData: { files }
      };
    });

    build.onLoad({ filter: /.*/, namespace: 'multi-entry' }, (args) => {
      const files = args.pluginData.files;
      const contents = files.map((file, index) => {
        const filePath = file.trim();
        // Check if file exists and read it directly
        if (fs.existsSync(filePath)) {
          return fs.readFileSync(filePath, 'utf8');
        } else {
          console.warn(`Warning: File not found: ${filePath}`);
          return `// File not found: ${filePath}`;
        }
      }).join('\n\n');
      
      return {
        contents,
        loader: 'js'
      };
    });
  }
};

// Enhanced bundling function
async function buildWithEsbuild(config) {
  const { input, output, minify = false } = config;
  
  console.log(`🚀 Bundling: ${input.join(', ')} -> ${output}`);
  
  try {
    let entryPoint;
    if (input.length === 1) {
      entryPoint = input[0];
    } else {
      // Use virtual multi-entry for multiple files
      entryPoint = `multi-entry:${input.join(',')}`;
    }
    
    const result = await esbuild.build({
      entryPoints: [entryPoint],
      bundle: true,
      minify: minify,
      treeShaking: true,
      target: 'es2017',
      format: 'iife',
      platform: 'browser',
      outfile: output,
      sourcemap: false,
      plugins: input.length > 1 ? [multiEntryPlugin] : [],
      // Optimize for browser
      legalComments: 'none',
      // Don't bundle external dependencies (Alpine.js etc)
      external: [], // We'll handle Alpine separately
      // Better minification
      keepNames: false,
      write: true,
      logLevel: 'silent'
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
  // Alpine.js library - just copy the file
  alpine: {
    input: ['node_modules/alpinejs/dist/cdn.min.js'],
    output: 'data/js/alpine.js',
    minify: false, // Already minified
  },

  alpineProd: {
    input: ['node_modules/alpinejs/dist/cdn.min.js'],
    output: 'data/js/alpine.js',
    minify: false, // Already minified
  },

  // Common app bundle 
  common: {
    input: [
      'src/js/config.js',
      'src/js/icons.js', 
      'src/js/app-common.js'
    ],
    output: 'data/js/app-common.js',
    minify: false,
  },

  commonProd: {
    input: [
      'src/js/config.js',
      'src/js/icons.js',
      'src/js/app-common.js'
    ],
    output: 'data/js/app-common.js',
    minify: true,
  },

  // Page-specific bundles
  index: {
    input: [
      'src/js/index-alpine-store.js',
      'src/js/index-api.js'
    ],
    output: 'data/js/page-index.js',
    minify: false,
  },

  indexProd: {
    input: [
      'src/js/index-alpine-store.js', 
      'src/js/index-api.js'
    ],
    output: 'data/js/page-index.js',
    minify: true,
  },

  settings: {
    input: [
      'src/js/settings-alpine-store.js',
      'src/js/settings-api.js'
    ],
    output: 'data/js/page-settings.js',
    minify: false,
  },

  settingsProd: {
    input: [
      'src/js/settings-alpine-store.js',
      'src/js/settings-api.js'
    ],
    output: 'data/js/page-settings.js',
    minify: true,
  },

  diagnostics: {
    input: [
      'src/js/diagnostics-alpine-store.js',
      'src/js/diagnostics-api.js'
    ],
    output: 'data/js/page-diagnostics.js',
    minify: false,
  },

  diagnosticsProd: {
    input: [
      'src/js/diagnostics-alpine-store.js',
      'src/js/diagnostics-api.js'
    ],
    output: 'data/js/page-diagnostics.js',
    minify: true,
  },

  setup: {
    input: [
      'src/js/setup-alpine-store.js',
      'src/js/setup-api.js'
    ],
    output: 'data/js/page-setup.js',
    minify: false,
  },

  setupProd: {
    input: [
      'src/js/setup-alpine-store.js',
      'src/js/setup-api.js'
    ],
    output: 'data/js/page-setup.js',
    minify: true,
  },

  '404': {
    input: ['src/js/404-alpine-store.js'],
    output: 'data/js/page-404.js',
    minify: false,
  },

  '404Prod': {
    input: ['src/js/404-alpine-store.js'],
    output: 'data/js/page-404.js',
    minify: true,
  },

  // Device settings page
  device: {
    input: [
      'src/js/settings-api.js',
      'src/js/page-settings-device.js'
    ],
    output: 'data/js/page-settings-device.js',
    minify: false,
  },

  deviceProd: {
    input: [
      'src/js/settings-api.js',
      'src/js/page-settings-device.js'
    ],
    output: 'data/js/page-settings-device.js',
    minify: true,
  },

  // WiFi settings page
  wifi: {
    input: [
      'src/js/settings-api.js',
      'src/js/page-settings-wifi.js'
    ],
    output: 'data/js/page-settings-wifi.js',
    minify: false,
  },

  wifiProd: {
    input: [
      'src/js/settings-api.js',
      'src/js/page-settings-wifi.js'
    ],
    output: 'data/js/page-settings-wifi.js',
    minify: true,
  },

  // MQTT settings page
  mqtt: {
    input: [
      'src/js/settings-api.js',
      'src/js/page-settings-mqtt.js'
    ],
    output: 'data/js/page-settings-mqtt.js',
    minify: false,
  },

  mqttProd: {
    input: [
      'src/js/settings-api.js',
      'src/js/page-settings-mqtt.js'
    ],
    output: 'data/js/page-settings-mqtt.js',
    minify: true,
  },

  // Settings overview page
  overview: {
    input: [
      'src/js/page-settings-overview.js'
    ],
    output: 'data/js/page-settings-overview.js',
    minify: false,
  },

  overviewProd: {
    input: [
      'src/js/page-settings-overview.js'
    ],
    output: 'data/js/page-settings-overview.js',
    minify: true,
  },

  // Memos settings page
  memos: {
    input: [
      'src/js/page-settings-memos.js'
    ],
    output: 'data/js/page-settings-memos.js',
    minify: false,
  },

  memosProd: {
    input: [
      'src/js/page-settings-memos.js'
    ],
    output: 'data/js/page-settings-memos.js',
    minify: true,
  },

  // Buttons settings page
  buttons: {
    input: [
      'src/js/settings-api.js',
      'src/js/page-settings-buttons.js'
    ],
    output: 'data/js/page-settings-buttons.js',
    minify: false,
  },

  buttonsProd: {
    input: [
      'src/js/settings-api.js',
      'src/js/page-settings-buttons.js'
    ],
    output: 'data/js/page-settings-buttons.js',
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
    
    // Special handling for Alpine.js bundle - just copy
    if (configName === 'alpine') {
      const sourceFile = config.input[0];
      const targetFile = config.output;
      
      if (fs.existsSync(sourceFile)) {
        fs.copyFileSync(sourceFile, targetFile);
        const stats = fs.statSync(targetFile);
        console.log(`✅ ${targetFile} (${stats.size} bytes) - copied`);
      } else {
        throw new Error(`Alpine.js file not found: ${sourceFile}`);
      }
    } else {
      await buildWithEsbuild(config);
    }
  } catch (error) {
    console.error(`❌ Error building ${configName}:`, error);
    process.exit(1);
  }
}

async function buildAll(production = false) {
  const suffix = production ? 'Prod' : '';
  const configs = [
    `alpine${suffix}`,
    `common${suffix}`,
    `index${suffix}`,
    `settings${suffix}`,
    `diagnostics${suffix}`, 
    `setup${suffix}`,
    `404${suffix}`,
    `device${suffix}`,
    `wifi${suffix}`,
    `mqtt${suffix}`,
    `overview${suffix}`,
    `memos${suffix}`,
    `buttons${suffix}`
  ];

  console.log(`Building all configs (production: ${production})...`);
  
  for (const config of configs) {
    await build(config);
  }
  
  console.log('🎉 All builds completed successfully!');
}

// Command line interface
if (require.main === module) {
  const args = process.argv.slice(2);
  const command = args[0];

  if (command === 'all') {
    buildAll(false);
  } else if (command === 'prod') {
    buildAll(true);  
  } else if (command && buildConfigs[command]) {
    build(command);
  } else {
    console.log('Usage:');
    console.log('  node esbuild.config.js all     # Build all configs (dev)');
    console.log('  node esbuild.config.js prod    # Build all configs (production)');
    console.log('  node esbuild.config.js <name>  # Build specific config');
    console.log('');
    console.log('Available configs:', Object.keys(buildConfigs).join(', '));
  }
}

module.exports = { buildConfigs, build, buildAll };