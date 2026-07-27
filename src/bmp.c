#include "bmp.h"

#include "efi.h"
#include "efilib.h"

BmpStatus loadBmp(UINT8 *buffer, UINTN size, BmpImage *image) {
  BmpHeader header;

  if (size < sizeof(BmpHeader))
    return BmpStatusInvalidSize;
  uefi_call_wrapper(BS->CopyMem, 3, &header, buffer, sizeof(BmpHeader));

  if (header.magic != 0x4D42)
    return BmpStatusInvalidMagic;

  if (header.offset > size)
    return BmpStatusInvalidSize;

  if (size < sizeof(BmpHeader) + sizeof(BmpInformationHeader))
    return BmpStatusInvalidSize;

  BmpInformationHeader info;
  uefi_call_wrapper(BS->CopyMem, 3, &info, buffer + sizeof(BmpHeader),
                    sizeof(info));

  if (info.size != sizeof(BmpInformationHeader))
    return BmpStatusUnsupportedFormat;
  if (info.planes != 1)
    return BmpStatusUnsupportedFormat;
  if (info.bits != 24)
    return BmpStatusUnsupportedFormat;
  if (info.compression != 0)
    return BmpStatusUnsupportedFormat;

  if (info.width <= 0 || info.height <= 0)
    return BmpStatusInvalidBmp;

  UINTN height = info.height;
  UINTN width = info.width;
  UINTN stride = ((width * 3) + 3) & ~3;
  UINTN imageSize = stride * height;

  if (header.offset + imageSize > size)
    return BmpStatusInvalidSize;

  UINT8 *pixels = buffer + header.offset;

  UINTN deserializedImageSize =
      width * height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *deserializedImage;

  EFI_STATUS status =
      uefi_call_wrapper(BS->AllocatePool, 3, EfiBootServicesData,
                        deserializedImageSize, (void **)&deserializedImage);
  if (EFI_ERROR(status)) {
    Print(u"woops");
    return BmpStatusAllocationError;
  }

  for (UINTN y = 0; y < height; y++) {
    UINTN row = height - 1 - y;
    UINT8 *pRow = pixels + (row * stride);

    for (UINTN x = 0; x < width; x++) {
      UINT8 blue = pRow[x * 3 + 0];
      UINT8 green = pRow[x * 3 + 1];
      UINT8 red = pRow[x * 3 + 2];

      UINTN outIndex = (y * width) + x;

      deserializedImage[outIndex].Blue = blue;
      deserializedImage[outIndex].Green = green;
      deserializedImage[outIndex].Red = red;
      deserializedImage[outIndex].Reserved = 0;
    }
  }

  image->width = width;
  image->height = width;
  image->pixels = deserializedImage;

  return BmpStatusOk;
}
