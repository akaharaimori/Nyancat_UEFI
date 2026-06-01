# 🐱🌈🔊 UEFI-Nyancat

Run the legendary **Nyan Cat** on bare-metal UEFI! 

This is a standalone, lightweight UEFI application (`.efi`) that plays the iconic Nyancat melody using the motherboard's **PC Speaker (8253 PIT)** while rendering a **100% flicker-free, double-buffered, pixel-perfect integer-scaled** animation on your screen using the UEFI Graphics Output Protocol (GOP).

No EDK2, no gnu-efi, no heavy libraries—just pure, raw C and assembly compiled natively with standard GCC.

## 📺 Demo (Running on Bare Metal!)

Here is a photo of the UEFI application running on real hardware:

![UEFI Nyancat running on real hardware](demo/IMG_20260601_185432.jpg)

Here is a video demonstrating the smooth, 100% flicker-free graphics and the nostalgic PC speaker sound in action on YouTube:

[![Watch UEFI Nyancat Demo on YouTube](https://img.youtube.com/vi/ZY-7qCbnmY4/0.jpg)](https://www.youtube.com/watch?v=ZY-7qCbnmY4)

## ✨ Features

- **🎹 Bare-metal PIT Sound Driver**: Direct hardware level programming of the 8253/8254 Programmable Interval Timer (PIT) Channel 2 at I/O ports `0x43` and `0x42` to produce high-precision square-wave beeps.
- **🚀 100% Flicker-free Graphics**: Unlike basic UEFI renderers that suffer from massive flickering due to slow VRAM writes, this project implements an in-RAM double buffer with a custom **Run-Length Encoded (RLE)** decoder. Frames are rendered entirely in RAM and pushed to VRAM in a single, hardware-accelerated BLT (`EfiBltBufferToVideo`) transfer.
- **📐 Pixel-Perfect Integer Scaling**: Automatically queries the native resolution of your monitor via GOP and calculates the maximum integer scaling factor (e.g., 6x scale on 1080p) to keep the pixel-art edges razor-sharp.
- **⏱️ Non-Blocking Real-time Game Loop**: Orchestrated via a millisecond-precision non-blocking timing loop using `gBS->Stall` to run audio notes and animation frames in perfect parallel coordination.
- **🎹 Responsive Safe-Exit**: Listens non-blockingly for any keystroke. Pressing any key will immediately mute the motherboard speaker and cleanly return to the UEFI shell.

---

## 💿 Quick Start (How to Run)

### Method 1: Physical PC via USB (Highly Recommended!)

1. Format a USB Flash Drive to **FAT32** (UEFI only recognizes FAT32).
2. Create a folder named `EFI\BOOT` on the USB drive.
3. Copy **`nyancat.efi`** into that folder and rename it to **`BOOTX64.EFI`**.
   - Path structure: `(USB Drive):\EFI\BOOT\BOOTX64.EFI`
4. Plug the USB drive into your target PC and boot from it (choose UEFI Boot in your BIOS/Boot Menu).
5. Watch the retro magic happen on bare metal! Press any key to stop and exit.

### Method 2: QEMU Virtual Machine

Ensure you have QEMU and the OVMF (UEFI firmware) image installed:

```bash
# Create directory structure
mkdir -p uefi_root/EFI/BOOT

# Copy and rename target binary
cp nyancat.efi uefi_root/EFI/BOOT/BOOTX64.EFI

# Launch QEMU with PC Speaker sound emulation enabled
qemu-system-x86_64 -bios OVMF.fd -hda fat:rw:uefi_root -soundhw pcspk
```

---

## 🛠️ Build from Source

This project compiles seamlessly on Windows using the **MinGW-w64 GCC** toolchain.

### Prerequisites

- GCC (MinGW-w64 supporting x86_64)
- Python 3 (with `Pillow` library installed for compressing GIF assets)

### Compilation

Just run the automated PowerShell build script:

```powershell
# Bypasses local execution policies and builds nyancat.efi instantly
powershell -ExecutionPolicy Bypass -File .\build.ps1
```

Under the hood, `build.ps1` runs:
1. `compress_gif.py` to compress the GIF frames into `nyancat_frames.h`.
2. `parse_audio.py` to compile the notes sheet into `nyancat_audio.h`.
3. `gcc` to link and create the UEFI PE executable:
   ```bash
   gcc -O3 -shared -nostdlib -mno-red-zone -fno-stack-protector -fno-builtin -Wl,--entry=efi_main -Wl,--subsystem,10 -o nyancat.efi main.c
   ```

---

## 📜 Credits & Licensing

This project is open-source and respects the intellectual property of the creators who made the legendary meme possible.

### Original Meme Creators
- **Original Animation (Pop-Tart Cat)**: Created by [PRguitarman (Chris Torres)](https://twitter.com/PRguitarman) in 2011.
- **Original Theme Song (Nyanyanyanyanyanyanya!)**: Composed by [daniwellP](https://daniwell.com/).

### Asset Source & Licenses
- **Melody data**: Parsed from the Arduino music sketch by [electricmango](https://github.com/electricmango/Arduino-Music-Project), licensed under **Creative Commons Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)**.
- **GIF frame assets**: Extracted from the repository by [TakWolf](https://github.com/TakWolf/NyanCatEverywhere), licensed under the **Apache License 2.0** (Copyright 2015 TakWolf).

### Code License
All custom UEFI headers, C timing loops, and PC speaker assembly drivers written for this project are licensed under the **MIT License**. Feel free to use, modify, and star this repository! ⭐
