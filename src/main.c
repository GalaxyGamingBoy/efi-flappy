#include <efi.h>
#include <efilib.h>

#include "bmp.h"
#include "rand.h"

BmpImage g_imgBird;
BmpImage g_imgMystery;

INT32 g_birdX = 0;
INT32 g_birdY = 0;

INT32 g_mysteryX[16] = {0};
INT32 g_mysteryY[16] = {0};

EFI_RNG_PROTOCOL *rng;

//
// Logs to the serial output of qemu
// Does not mess up the screen graphics
//
void log_serial(EFI_SERIAL_IO_PROTOCOL *serial, CONST CHAR16 *text) {
  UINTN sz = 1;
  while (*text) {
    uefi_call_wrapper(serial->Write, 3, serial, &sz, (void *)text);
    text++;
  }
}

//
// Clears the screen framebuffer using memset
//
void clear_screen(CONST UINT32 width, CONST UINT32 height,
                  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *fb) {
  uefi_call_wrapper(BS->SetMem, 3, fb,
                    height * width * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL), 0);
}

void draw_image(INT32 x, INT32 y, BmpImage image, CONST UINT32 fbW,
                CONST UINT32 fbH, EFI_GRAPHICS_OUTPUT_BLT_PIXEL *fb) {
  for (UINTN iy = 0; iy < image.height; iy++)
    for (UINTN ix = 0; ix < image.width; ix++) {
      INT32 px = x + ix;
      INT32 py = y + iy;

      if (px >= fbW)
        continue;
      if (py >= fbH)
        continue;

      UINTN ipxl = iy * image.width + ix;
      UINTN fpxl = (iy + y) * fbW + (ix + x);

      fb[fpxl] = image.pixels[ipxl];
    }
}

//
// Draws the game state to the screen
//
void draw(CONST UINT32 width, CONST UINT32 height,
          EFI_GRAPHICS_OUTPUT_BLT_PIXEL *fb,
          EFI_GRAPHICS_OUTPUT_PROTOCOL *gfx) {
  clear_screen(width, height, fb);

  draw_image(g_birdX, g_birdY, g_imgBird, width, height, fb);
  for (int i = 0; i < 16; i++)
    draw_image(g_mysteryX[i], g_mysteryY[i], g_imgMystery, width, height, fb);

  // Blit FB to Video
  EFI_STATUS status = uefi_call_wrapper(
      gfx->Blt, 10, gfx, fb, EfiBltBufferToVideo, 0, 0, 0, 0, width, height, 0);

  if (EFI_ERROR(status))
    Print(u"ERR: Failed to blit graphics\n");
}

float g_birdVY = 0.0f;

void randomize_block(int index, UINT32 width, UINT32 height) {
  int x = rand() % width + g_birdX * 3;
  int y = rand() % width;

  g_mysteryX[index] = x;
  g_mysteryY[index] = y;
}

void update(EFI_INPUT_KEY key, UINT32 width, UINT32 height) {
  g_birdVY += 0.40f;

  if (key.ScanCode == SCAN_UP)
    g_birdVY = -8.0f;

  g_birdY += g_birdVY;

  for (int i = 0; i < 16; i++) {
    if (g_mysteryX[i] <= -g_imgMystery.width)
      randomize_block(i, width, height);

    g_mysteryX[i] -= 2;

    if (g_birdX < g_mysteryX[i] + g_imgMystery.width &&
        g_birdX + g_imgBird.width > g_mysteryX[i] &&
        g_birdY < g_mysteryY[i] + g_imgMystery.height &&
        g_birdY + g_imgBird.height > g_mysteryY[i]) {
      uefi_call_wrapper(gRT->ResetSystem, 4, EfiResetCold, EFI_SUCCESS, 0,
                        NULL);
    }
  }
}

EFI_STATUS GetInfo(EFI_FILE_HANDLE file, UINT64 *out) {
  EFI_STATUS status;

  UINTN size = 0;
  status =
      uefi_call_wrapper(file->GetInfo, 4, file, &GenericFileInfo, &size, NULL);

  if (status != EFI_BUFFER_TOO_SMALL)
    goto done;

  EFI_FILE_INFO *FileInfo = NULL;
  status = uefi_call_wrapper(BS->AllocatePool, 3, EfiBootServicesData, size,
                             (void **)&FileInfo);
  if (EFI_ERROR(status))
    goto done;

  status = uefi_call_wrapper(file->GetInfo, 4, file, &GenericFileInfo, &size,
                             FileInfo);
  if (EFI_ERROR(status))
    goto done;

  *out = FileInfo->FileSize;
done:
  if (FileInfo)
    uefi_call_wrapper(BS->FreePool, 1, FileInfo);
  return status;
}

EFI_STATUS LoadImage(CHAR16 *filename, EFI_FILE_HANDLE root, BmpImage *image) {
  EFI_STATUS status;
  EFI_FILE_HANDLE bmp = NULL;

  status = uefi_call_wrapper(root->Open, 5, root, &bmp, filename,
                             EFI_FILE_MODE_READ, 9);
  if (EFI_ERROR(status)) {
    Print(u"ERR: Failed to open BMP file, %d\n", status);
    goto done;
  }

  UINT64 size;
  status = GetInfo(bmp, &size);
  if (EFI_ERROR(status)) {
    Print(u"ERR: Failed to get info from BMP file, %d\n", status);
    goto done;
  }

  UINT8 *buf = NULL;
  status = uefi_call_wrapper(BS->AllocatePool, 3, EfiBootServicesData, size,
                             (void **)&buf);
  if (EFI_ERROR(status)) {
    Print(u"ERR: Failed to allocate BMP file memory, %d\n", status);
    goto done;
  }

  status = uefi_call_wrapper(bmp->Read, 3, bmp, &size, buf);
  if (EFI_ERROR(status)) {
    Print(u"ERR: Failed to read BMP file, %d\n", status);
    goto done;
  }

  status = loadBmp(buf, size, image);
  if (status != BmpStatusOk) {
    Print(u"ERR: Failed to parse BMP file, %d\n", status);
    goto done;
  }

done:
  if (bmp)
    uefi_call_wrapper(bmp->Close, 1, bmp);
  if (buf)
    uefi_call_wrapper(BS->FreePool, 1, buf);
  return status;
}

void FreeImage(BmpImage image) {
  uefi_call_wrapper(BS->FreePool, 1, image.pixels);
}

EFI_STATUS
EFIAPI
efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
  InitializeLib(ImageHandle, SystemTable);

  EFI_TIME Time;
  EFI_TIME_CAPABILITIES Capabilities;

  uefi_call_wrapper(gRT->GetTime, 2, &Time, &Capabilities);

  srand(((UINT64)Time.Year << 48) | ((UINT64)Time.Month << 40) |
        ((UINT64)Time.Day << 32) | ((UINT64)Time.Hour << 24) |
        ((UINT64)Time.Minute << 16) | ((UINT64)Time.Second << 8) |
        Time.Nanosecond);

  // Load protocols
  EFI_STATUS status;
  status = uefi_call_wrapper(BS->SetWatchdogTimer, 4, 0, 0, 0, NULL);
  if (EFI_ERROR(status)) {
    Print(u"ERR: Couldn't stop watchdog timer\n");
    return EFI_DEVICE_ERROR;
  }

  EFI_SERIAL_IO_PROTOCOL *serial;
  status = uefi_call_wrapper(BS->LocateProtocol, 3, &SerialIoProtocol, NULL,
                             (void **)&serial);
  if (EFI_ERROR(status)) {
    Print(u"ERR: Failed to get serial protocol\n");
    return EFI_NOT_FOUND;
  }

  EFI_LOADED_IMAGE *loadedImage;
  status = uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle,
                             &LoadedImageProtocol, (void **)&loadedImage);
  if (EFI_ERROR(status)) {
    Print(u"ERR: Failed to get the loaded image protocol\n");
    return EFI_NOT_FOUND;
  }

  EFI_FILE_IO_INTERFACE *fileSystem;
  status = uefi_call_wrapper(BS->HandleProtocol, 3, loadedImage->DeviceHandle,
                             &FileSystemProtocol, (void **)&fileSystem);
  if (EFI_ERROR(status)) {
    Print(u"ERR: Failed to get the file system protocol\n");
    return EFI_NOT_FOUND;
  }

  EFI_FILE_HANDLE fileSystemRoot;
  status =
      uefi_call_wrapper(fileSystem->OpenVolume, 2, fileSystem, &fileSystemRoot);
  if (EFI_ERROR(status)) {
    Print(u"ERR: Failed to get the root filesystem\n");
    return EFI_DEVICE_ERROR;
  }

  EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics;
  status = uefi_call_wrapper(BS->LocateProtocol, 3, &GraphicsOutputProtocol,
                             NULL, (void **)&graphics);
  if (EFI_ERROR(status)) {
    Print(u"ERR: Couldn't find the graphics protocol\n");
    return EFI_NOT_FOUND;
  }

  // Create framebuffer
  CONST UINT32 width = graphics->Mode->Info->HorizontalResolution;
  CONST UINT32 height = graphics->Mode->Info->VerticalResolution;

  EFI_GRAPHICS_OUTPUT_BLT_PIXEL *framebuffer;
  UINTN fbSz = width * height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
  status = uefi_call_wrapper(BS->AllocatePool, 3, EfiBootServicesData, fbSz,
                             (void **)&framebuffer);
  if (EFI_ERROR(status)) {
    Print(u"ERR: Failed to allocate framebuffer pool\n");
    return EFI_DEVICE_ERROR;
  }

  // Load images
  status = LoadImage(u"flappyres\\bird.bmp", fileSystemRoot, &g_imgBird);
  if (EFI_ERROR(status))
    return status;
  status = LoadImage(u"flappyres\\block.bmp", fileSystemRoot, &g_imgMystery);
  if (EFI_ERROR(status))
    return status;

  // Set Bird Position
  g_birdX = 48;
  g_birdY = (height / 2) - (g_imgBird.height / 2);

  for (int i = 0; i < 16; i++)
    randomize_block(i, width, height);

  // Main Loop
  BOOLEAN exit = FALSE;
  while (!exit) {
    // 1. Process Inputes
    EFI_INPUT_KEY keyPressed;
    uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &keyPressed);

    if (keyPressed.ScanCode == SCAN_ESC)
      exit = TRUE;

    // 2. Update
    update(keyPressed, width, height);

    if (g_birdY > height) {
      uefi_call_wrapper(gRT->ResetSystem, 4, EfiResetCold, EFI_SUCCESS, 0,
                        NULL);
    }

    // 3. Draw
    draw(width, height, framebuffer, graphics);
  }

  FreeImage(g_imgBird);
  uefi_call_wrapper(BS->FreePool, 1, framebuffer);

  return EFI_SUCCESS;
}
