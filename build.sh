#!/bin/sh
# Compile the library and the example with every warning the toolchain has for
# this kind of code. No hardware, no controller headers: it checks the source,
# not a port. Override the compiler with CC=gcc CC=clang CC=arm-none-eabi-gcc.
set -e

CC=${CC:-cc}
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT
CFLAGS="-std=c99 -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion -Wshadow -Wundef -Wwrite-strings -Werror -I src"

for source in src/sd_card.c src/sd_card_host.c examples/sd_card_host_template.c; do
    echo "compiling $source"
    "$CC" $CFLAGS -c "$source" -o "$OUT/$(basename "$source" .c).o"
done

echo "ok"
