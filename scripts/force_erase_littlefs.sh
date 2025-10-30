#!/bin/bash
# Force erase LittleFS partition by writing zeros
# This clears any corrupt metadata that prevents format from working

PORT="/dev/cu.usbmodem134301"
PARTITION_OFFSET="0x510000"
PARTITION_SIZE="0x2F0000"  # 3,080,192 bytes

echo "=== Force Erase LittleFS Partition ==="
echo ""
echo "Creating erased (all 0xFF) image file..."
# Flash erase state is 0xFF, not 0x00
tr '\000' '\377' < /dev/zero | dd of=/tmp/blank_partition.bin bs=1024 count=3008 2>/dev/null

echo ""
echo "Writing erased data (0xFF) to flash at ${PARTITION_OFFSET}..."
echo "This will take ~30 seconds..."
esptool --port "${PORT}" write_flash "${PARTITION_OFFSET}" /tmp/blank_partition.bin

echo ""
echo "✅ LittleFS partition erased with 0xFF (flash erase state)"
echo ""
echo "Next steps:"
echo "  1. Upload firmware: pio run -e s3-pcb-dev -t upload"
echo "  2. Upload filesystem: pio run -e s3-pcb-dev -t uploadfs"
echo ""
echo "The firmware should now be able to format LittleFS successfully."
