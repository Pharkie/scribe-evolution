#!/bin/bash
# Debug script: Read partition table from ESP32-S3 flash and compare with expected

PORT="/dev/cu.usbmodem134301"
PARTITION_OFFSET="0x8000"  # Standard ESP32 partition table location

echo "=== ESP32-S3 Flash Partition Debug ==="
echo ""
echo "1. Reading partition table from flash at ${PARTITION_OFFSET}..."
esptool --port "${PORT}" read_flash "${PARTITION_OFFSET}" 0xC00 /tmp/esp32_partitions.bin 2>&1 | grep -v "Deprecated"

echo ""
echo "2. Decoding partition table binary..."
python3 << 'EOF'
import struct

with open('/tmp/esp32_partitions.bin', 'rb') as f:
    data = f.read()

print(f"{'Name':<12} {'Type':<8} {'SubType':<10} {'Offset':<12} {'Size':<12}")
print("-" * 60)

offset = 0
while offset < len(data):
    entry = data[offset:offset+32]
    if entry[0:2] == b'\xAA\x50':
        ptype = entry[2]
        subtype = entry[3]
        pos = struct.unpack('<I', entry[4:8])[0]
        size = struct.unpack('<I', entry[8:12])[0]
        name = entry[12:28].decode('utf-8', errors='ignore').rstrip('\x00')

        type_str = {0: 'app', 1: 'data'}.get(ptype, f'0x{ptype:02x}')
        if ptype == 0:
            subtype_str = {0: 'factory', 16: 'ota_0'}.get(subtype, f'0x{subtype:02x}')
        elif ptype == 1:
            subtype_str = {1: 'ota', 2: 'nvs', 130: 'spiffs'}.get(subtype, f'0x{subtype:02x}')
        else:
            subtype_str = f'0x{subtype:02x}'

        print(f"{name:<12} {type_str:<8} {subtype_str:<10} 0x{pos:08x}   0x{size:08x}")
    offset += 32
    if offset >= 0xC00:
        break
EOF

echo ""
echo "3. Reading first 256 bytes of LittleFS partition (0x510000)..."
esptool --port "${PORT}" read_flash 0x510000 256 /tmp/littlefs_start.bin 2>&1 | grep -v "Deprecated"
echo ""
echo "First 128 bytes of LittleFS partition:"
xxd -l 128 /tmp/littlefs_start.bin

echo ""
echo "=== Analysis ==="
echo "If partition table shows 'littlefs' with correct offset/size: partition exists"
echo "If LittleFS data shows all 0xFF: partition is erased (expected after erase_flash)"
echo "If LittleFS data shows other patterns: partition may have corrupt data"
