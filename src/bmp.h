#pragma once

#include "efi.h"
#include "efilib.h"

typedef struct __attribute__((packed)) {
  unsigned short int magic;
  unsigned int size;
  unsigned short int r1, r2;
  unsigned int offset;
} BmpHeader;

typedef struct __attribute__((packed)) {
  unsigned int size;
  int width, height;
  unsigned short int planes;
  unsigned short int bits;
  unsigned int compression;
  unsigned int imagesize;
  int resolutionX, resolutionY;
  unsigned int numColors;
  unsigned int impColors;
} BmpInformationHeader;

typedef enum {
  BmpStatusOk,
  BmpStatusInvalidSize,
  BmpStatusInvalidMagic,
  BmpStatusAllocationError,
  BmpStatusUnsupportedFormat,
  BmpStatusInvalidBmp
} BmpStatus;

typedef struct {
  int width;
  int height;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *pixels;
} BmpImage;

BmpStatus loadBmp(UINT8 *buffer, UINTN size, BmpImage *image);
