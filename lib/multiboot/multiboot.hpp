#ifndef MULTIBOOT_H
#define MULTIBOOT_H

#include <stdint.h>
#include <driver/spi_master.h>

#include "FS.h"
#include "SPIFFS.h"

void initSPI();

uint32_t send(uint32_t command);

void multiboot(uint8_t* data, size_t len);
void multiboot(File &file, size_t len);

#endif
