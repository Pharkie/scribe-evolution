#!/usr/bin/env node

/**
 * GZIP Assets Build Script
 * Compresses all web assets for ESP32 serving with Content-Encoding: gzip
 */

const fs = require("fs");
const path = require("path");
const { createGzip } = require("zlib");
const { pipeline } = require("stream/promises");

// Assets to compress (web-served files only)
const COMPRESS_EXTENSIONS = [".html", ".css", ".js", ".json", ".svg", ".md"];
// Excluded: .ndjson, .txt, .pem (C++ code resources - never web-served)
// Excluded: .woff2 (already compressed format - gzipping adds overhead)
const MIN_SIZE_BYTES = 0; // Compress all files regardless of size for consistency

async function gzipFile(filePath) {
  const gzipPath = filePath + ".gz";

  try {
    const stats = await fs.promises.stat(filePath);

    // Skip very small files
    if (stats.size < MIN_SIZE_BYTES) {
      console.log(
        `⏭️  Skipping ${filePath} (${stats.size} bytes < ${MIN_SIZE_BYTES} threshold)`,
      );
      return;
    }

    // Compress the file
    const source = fs.createReadStream(filePath);
    const destination = fs.createWriteStream(gzipPath);
    const gzip = createGzip({ level: 9 }); // Maximum compression

    await pipeline(source, gzip, destination);

    const gzipStats = await fs.promises.stat(gzipPath);
    const reduction = (
      ((stats.size - gzipStats.size) / stats.size) *
      100
    ).toFixed(1);

    console.log(`✅ ${path.relative(process.cwd(), filePath)}`);
    console.log(
      `   ${stats.size.toLocaleString()} → ${gzipStats.size.toLocaleString()} bytes (${reduction}% reduction)`,
    );

    return {
      original: stats.size,
      compressed: gzipStats.size,
      reduction: parseFloat(reduction),
    };
  } catch (error) {
    console.error(`❌ Failed to compress ${filePath}:`, error.message);
    return null;
  }
}

async function findFiles(dir, extensions) {
  const files = [];

  async function scanDir(currentDir) {
    const entries = await fs.promises.readdir(currentDir, {
      withFileTypes: true,
    });

    for (const entry of entries) {
      const fullPath = path.join(currentDir, entry.name);

      if (entry.isDirectory()) {
        await scanDir(fullPath);
      } else if (entry.isFile()) {
        const ext = path.extname(entry.name).toLowerCase();
        if (extensions.includes(ext)) {
          files.push(fullPath);
        }
      }
    }
  }

  await scanDir(dir);
  return files;
}

async function main() {
  console.log("🗜️  GZIP Assets Build Script\n");

  const dataDir = path.join(__dirname, "../data");

  // Check if data directory exists
  if (!fs.existsSync(dataDir)) {
    console.error("❌ Data directory not found:", dataDir);
    process.exit(1);
  }

  // Find all compressible files
  console.log("📁 Scanning for compressible assets...");
  const files = await findFiles(dataDir, COMPRESS_EXTENSIONS);

  if (files.length === 0) {
    console.log("ℹ️  No compressible files found.");
    return;
  }

  console.log(`📦 Found ${files.length} files to compress\n`);

  // Compress all files
  const results = [];
  for (const filePath of files) {
    const result = await gzipFile(filePath);
    if (result) {
      results.push(result);
    }
  }

  // Summary statistics
  if (results.length > 0) {
    const totalOriginal = results.reduce((sum, r) => sum + r.original, 0);
    const totalCompressed = results.reduce((sum, r) => sum + r.compressed, 0);
    const averageReduction =
      results.reduce((sum, r) => sum + r.reduction, 0) / results.length;

    console.log("\n📊 Compression Summary:");
    console.log(`   Original size: ${totalOriginal.toLocaleString()} bytes`);
    console.log(
      `   Compressed size: ${totalCompressed.toLocaleString()} bytes`,
    );
    console.log(
      `   Total savings: ${(totalOriginal - totalCompressed).toLocaleString()} bytes`,
    );
    console.log(`   Average reduction: ${averageReduction.toFixed(1)}%`);
    console.log(`   Files compressed: ${results.length}/${files.length}`);
  }

  console.log("\n✅ GZIP compression complete!");
}

if (require.main === module) {
  main().catch((error) => {
    console.error("❌ Script failed:", error);
    process.exit(1);
  });
}

module.exports = { gzipFile, findFiles };
