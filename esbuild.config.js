const esbuild = require('esbuild');
const fs = require('fs');
const path = require('path');

// Simple file concatenation and minification (like terser does)
async function concatAndMinify(inputFiles, outputFile, minify = false) {
  console.log(`Processing: ${inputFiles.join(', ')} -> ${outputFile}`);
  
  // Read and concatenate all input files
  let content = '';
  for (const file of inputFiles) {
    if (fs.existsSync(file)) {
      content += fs.readFileSync(file, 'utf8') + '\n';
    } else {
      console.warn(`Warning: File not found: ${file}`);
    }
  }
  
  // Use esbuild to process the concatenated content
  const result = await esbuild.transform(content, {
    minify: minify,
    target: 'es2017',
    format: 'iife',
    sourcemap: false,
  });
  
  // Write output
  fs.writeFileSync(outputFile, result.code);
  
  const stats = fs.statSync(outputFile);
  console.log(`✅ ${outputFile} (${stats.size} bytes)`);
}

// Build configurations matching current package.json structure
const buildConfigs = {
  // Vendor bundle (Alpine.js) - just copy the file
  vendor: {
    input: ['node_modules/alpinejs/dist/cdn.min.js'],
    output: 'data/js/vendor.min.js',
    minify: false, // Already minified
  },

  // Common app bundle 
  common: {
    input: [
      'src/js/config.js',
      'src/js/icons.js', 
      'src/js/app-common.js'
    ],
    output: 'data/js/app-common.min.js',
    minify: false,
  },

  commonProd: {
    input: [
      'src/js/config.js',
      'src/js/icons.js',
      'src/js/app-common.js'
    ],
    output: 'data/js/app-common.min.js',
    minify: true,
  },

  // Page-specific bundles
  index: {
    input: [
      'src/js/index-alpine-store.js',
      'src/js/index-api.js'
    ],
    output: 'data/js/page-index.min.js',
    minify: false,
  },

  indexProd: {
    input: [
      'src/js/index-alpine-store.js', 
      'src/js/index-api.js'
    ],
    output: 'data/js/page-index.min.js',
    minify: true,
  },

  settings: {
    input: [
      'src/js/settings-alpine-store.js',
      'src/js/settings-api.js'
    ],
    output: 'data/js/page-settings.min.js',
    minify: false,
  },

  settingsProd: {
    input: [
      'src/js/settings-alpine-store.js',
      'src/js/settings-api.js'
    ],
    output: 'data/js/page-settings.min.js',
    minify: true,
  },

  diagnostics: {
    input: [
      'src/js/diagnostics-alpine-store.js',
      'src/js/diagnostics-api.js'
    ],
    output: 'data/js/page-diagnostics.min.js',
    minify: false,
  },

  diagnosticsProd: {
    input: [
      'src/js/diagnostics-alpine-store.js',
      'src/js/diagnostics-api.js'
    ],
    output: 'data/js/page-diagnostics.min.js',
    minify: true,
  },

  setup: {
    input: [
      'src/js/setup-alpine-store.js',
      'src/js/setup-api.js'
    ],
    output: 'data/js/page-setup.min.js',
    minify: false,
  },

  setupProd: {
    input: [
      'src/js/setup-alpine-store.js',
      'src/js/setup-api.js'
    ],
    output: 'data/js/page-setup.min.js',
    minify: true,
  },

  '404': {
    input: ['src/js/404-alpine-store.js'],
    output: 'data/js/page-404.min.js',
    minify: false,
  },

  '404Prod': {
    input: ['src/js/404-alpine-store.js'],
    output: 'data/js/page-404.min.js',
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
    await concatAndMinify(config.input, config.output, config.minify);
  } catch (error) {
    console.error(`❌ Error building ${configName}:`, error);
    process.exit(1);
  }
}

async function buildAll(production = false) {
  const suffix = production ? 'Prod' : '';
  const configs = [
    'vendor',
    `common${suffix}`,
    `index${suffix}`,
    `settings${suffix}`,
    `diagnostics${suffix}`, 
    `setup${suffix}`,
    `404${suffix}`
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