#!/bin/bash
# Debug script to print environment information

echo "=== Current Directory ==="
pwd

echo -e "\n=== Project Structure ==="
find .. -name "*.h" | grep -E 'usb_control.h|hurricane_hw_hal.h|usb_host_controller.h' | sort

echo -e "\n=== Include Paths ==="
echo "From test directory:"
for dir in "../lib/hurricane" "../lib/hurricane/core" "../lib/hurricane/usb" "../lib/hurricane/hw" "../lib/hurricane/hw/boards/dummy" ".."; do
  echo "  $dir exists: $(if [ -d "$dir" ]; then echo "Yes"; else echo "No"; fi)"
done

echo -e "\n=== Test File Content ==="
echo "test_usb_control.c includes:"
grep -n "#include" unit/test_usb_control.c

echo -e "\n=== Try direct compilation ==="
clang -I../lib/hurricane -I../lib/hurricane/core -I../lib/hurricane/usb -I../lib/hurricane/hw -I../lib/hurricane/hw/boards/dummy -I.. -Wall -Wextra -std=c11 -g -E unit/test_usb_control.c > /dev/null
if [ $? -eq 0 ]; then
  echo "Preprocessor ran successfully"
else
  echo "Preprocessor failed"
fi