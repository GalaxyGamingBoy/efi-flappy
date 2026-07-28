# EFI Flappy Bird Reimagination

<img width="480" height="312" alt="flappy-demo" src="https://github.com/user-attachments/assets/60f79db1-78d0-40f9-bca8-9e886d707714" />

A reimagined version of Flappy Bird adapted to the nature of UEFI with greater and more annoying consequences.
To be exact, instead of pipes I changed it to randomly generated blocks on the screen that the bird must avoid.
If the bird touches the block or goes above/below the screen, the whole system reboots! So, be careful!

The built version containing a bash script to run QEMU with the appropriate arguments can be found in a zip [here](https://github.com/GalaxyGamingBoy/efi-flappy/releases/tag/1.0), by downloading the `flappy.zip` file

## Quick start guide ( UNIX )

1. Download the latest release zip from above
2. Extract the archive
3. Make sure you have QEMU downloaded
4. Run the `run.sh` script that is bundled
5. QEMU should have started with a shell and the application preloaded

## Features

1. Bitmap graphics
2. Serial developer output
3. Custom bitmap parser and renderer
4. Custom textures via the UEFI filesystem
5. Configured GNU-EFI Makefile

## Useful Makefile Tasks

Run using: `make <task>`

| Task | Description |
| --- | --- |
| qemu | Builds latest version and starts a QEMU emulator |
| esp/ | Prepares the Efi System Partition folder for usage in QEMU |

By not passing any task, `make` simply builds the project.

## How it works

The application uses GNU-EFI to interop with UEFI Boot Services.
Using that the application requests the necessary protocols that are needed for execution,
such as the filesystem root handle (Simple File System Protocol)
to get assets and the **G**raphics **O**utput **P**rotocol to control the graphics.
Next, the application requests a application pool to use as a framebuffer that then using
**BL**ock **T**ransfer copies to the screen.

## Acknowledgements

This project is only made possibly by the incredible efforts of the people support GNU-EFI and it's development, the UEFI Forum and any other contributors.

> [!NOTE]
> This project is indepedant, unaffiliated, and not associated or endorsed by the UEFI Forum or any of its subsidiaries or its affialiates. Use of this library is at your own risk. This library is provided "as is" without warranties of any kind.
> 
