#include "uefi.h"
#include "nyancat_frames.h"
#include "nyancat_audio.h"

// Speaker control using direct inline assembly for x86_64
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static void speaker_on(unsigned int freq) {
    if (freq == 0) return;
    
    // Calculate frequency divisor for PIT Channel 2
    unsigned int divisor = 1193180 / freq;
    
    // Set PIT Channel 2 to square wave generator (mode 3)
    outb(0x43, 0xB6);
    
    // Write divisor (low byte first, then high byte)
    outb(0x42, (uint8_t)(divisor & 0xFF));
    outb(0x42, (uint8_t)((divisor >> 8) & 0xFF));
    
    // Turn on the speaker by setting bits 0 and 1 of Port 0x61
    uint8_t val = inb(0x61);
    if ((val & 0x03) != 0x03) {
        outb(0x61, val | 0x03);
    }
}

static void speaker_off(void) {
    // Turn off speaker by clearing bits 0 and 1 of Port 0x61
    uint8_t val = inb(0x61);
    outb(0x61, val & 0xFC);
}

// GOP GUID: 9042a9de-23dc-4a38-96fb-7aded080516a
static const EFI_GUID gop_guid = {
    0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}
};

// Global variables for scaling and Blt drawing
static EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL *blt_buffer = NULL;
static unsigned int scale = 1;
static unsigned int start_x = 0;
static unsigned int start_y = 0;
static unsigned int blt_width = 0;
static unsigned int blt_height = 0;

static void draw_frame(int frame_idx) {
    const RleFrame *frame = &nyancat_frames[frame_idx];
    unsigned int pixel_idx = 0;
    
    for (unsigned int run_idx = 0; run_idx < frame->num_runs; run_idx++) {
        RleRun run = frame->runs[run_idx];
        
        // Extract color components from 0x00RRGGBB (stored in little endian as Blue, Green, Red, Reserved)
        EFI_GRAPHICS_OUTPUT_BLT_PIXEL pixel;
        pixel.Blue = (uint8_t)(run.color & 0xFF);
        pixel.Green = (uint8_t)((run.color >> 8) & 0xFF);
        pixel.Red = (uint8_t)((run.color >> 16) & 0xFF);
        pixel.Reserved = 0;
        
        // Fill runs in Blt buffer, scaling on the fly
        for (unsigned short k = 0; k < run.count; k++) {
            unsigned int p = pixel_idx + k;
            unsigned int px = p % NYANCAT_WIDTH;
            unsigned int py = p / NYANCAT_WIDTH;
            
            // Draw scale * scale block in Blt buffer
            for (unsigned int dy = 0; dy < scale; dy++) {
                unsigned int dest_y = py * scale + dy;
                unsigned int dest_x_start = px * scale;
                EFI_GRAPHICS_OUTPUT_BLT_PIXEL *dest_line = blt_buffer + dest_y * blt_width + dest_x_start;
                
                for (unsigned int dx = 0; dx < scale; dx++) {
                    dest_line[dx] = pixel;
                }
            }
        }
        pixel_idx += run.count;
    }
    
    // Copy the prepared scaled frame buffer to the video memory (screen)
    gop->Blt(
        gop,
        blt_buffer,
        EfiBltBufferToVideo,
        0, 0,           // Source X, Y
        start_x, start_y, // Destination X, Y
        blt_width, blt_height,
        0               // Delta (0 means auto-calculated width * sizeof(Pixel))
    );
}

// EFI Application Entry Point
EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS status;
    
    // Clean and reset screen for initialization text
    SystemTable->ConOut->Reset(SystemTable->ConOut, BOOLEAN_FALSE);
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    
    // 16-bit unicode characters for console output
    CHAR16 init_msg[] = { '[', 'N', 'y', 'a', 'n', 'c', 'a', 't', ' ', 'U', 'E', 'F', 'I', ']', ' ', 'I', 'n', 'i', 't', 'i', 'a', 'l', 'i', 'z', 'i', 'n', 'g', '.', '.', '.', '\r', '\n', 0 };
    SystemTable->ConOut->OutputString(SystemTable->ConOut, init_msg);
    
    // 1. Locate Graphics Output Protocol (GOP)
    status = SystemTable->BootServices->LocateProtocol((EFI_GUID *)&gop_guid, NULL, (VOID **)&gop);
    if (status != EFI_SUCCESS || gop == NULL) {
        CHAR16 err_msg[] = { 'E', 'r', 'r', 'o', 'r', ':', ' ', 'F', 'a', 'i', 'l', 'e', 'd', ' ', 't', 'o', ' ', 'l', 'o', 'c', 'a', 't', 'e', ' ', 'G', 'O', 'P', '!', '\r', '\n', 0 };
        SystemTable->ConOut->OutputString(SystemTable->ConOut, err_msg);
        return status;
    }
    
    // Get screen resolutions from GOP (with safety checks)
    if (gop->Mode == NULL || gop->Mode->Info == NULL) {
        CHAR16 err_msg[] = { 'E', 'r', 'r', 'o', 'r', ':', ' ', 'I', 'n', 'v', 'a', 'l', 'i', 'd', ' ', 'G', 'O', 'P', ' ', 'm', 'o', 'd', 'e', '!', '\r', '\n', 0 };
        SystemTable->ConOut->OutputString(SystemTable->ConOut, err_msg);
        return EFI_DEVICE_ERROR;
    }
    
    unsigned int screen_width = gop->Mode->Info->HorizontalResolution;
    unsigned int screen_height = gop->Mode->Info->VerticalResolution;
    
    // 2. Compute maximum integer scale factor (keeping pixel art perfectly sharp!)
    unsigned int scale_x = screen_width / NYANCAT_WIDTH;
    unsigned int scale_y = screen_height / NYANCAT_HEIGHT;
    scale = (scale_x < scale_y) ? scale_x : scale_y;
    if (scale == 0) scale = 1; // Fallback
    
    // Size of the centered upscaled frame buffer
    blt_width = NYANCAT_WIDTH * scale;
    blt_height = NYANCAT_HEIGHT * scale;
    start_x = (screen_width - blt_width) / 2;
    start_y = (screen_height - blt_height) / 2;
    
    // 3. Allocate pool for BltBuffer in RAM (Double buffering)
    UINTN buffer_size = blt_width * blt_height * sizeof(EFI_GRAPHICS_OUTPUT_BLT_PIXEL);
    status = SystemTable->BootServices->AllocatePool(EfiBootServicesData, buffer_size, (VOID **)&blt_buffer);
    if (status != EFI_SUCCESS || blt_buffer == NULL) {
        CHAR16 err_msg[] = { 'E', 'r', 'r', 'o', 'r', ':', ' ', 'F', 'a', 'i', 'l', 'e', 'd', ' ', 't', 'o', ' ', 'a', 'l', 'l', 'o', 'c', 'a', 't', 'e', ' ', 'm', 'e', 'm', 'o', 'r', 'y', '!', '\r', '\n', 0 };
        SystemTable->ConOut->OutputString(SystemTable->ConOut, err_msg);
        return status;
    }
    
    // 4. Fill screen background with Nyancat deep-blue color (#003366)
    EFI_GRAPHICS_OUTPUT_BLT_PIXEL bg_pixel;
    bg_pixel.Blue = 102;  // 0x66
    bg_pixel.Green = 51;  // 0x33
    bg_pixel.Red = 0;     // 0x00
    bg_pixel.Reserved = 0;
    
    gop->Blt(
        gop,
        &bg_pixel,
        EfiBltVideoFill,
        0, 0,
        0, 0,
        screen_width, screen_height,
        0
    );
    
    // 5. Main Non-Blocking Loop
    unsigned long long current_time = 0;
    unsigned long long next_frame_time = 0;
    unsigned long long next_audio_event_time = 0;
    
    int audio_state = 0; // 0 = playing note, 1 = pause/silence between notes
    unsigned int current_note_idx = 0;
    unsigned int current_frame_idx = 0;
    
    while (1) {
        // Check for ANY key press to stop playback and exit
        EFI_INPUT_KEY key;
        status = SystemTable->ConIn->ReadKeyStroke(SystemTable->ConIn, &key);
        if (status == EFI_SUCCESS) {
            break;
        }
        
        // Audio Engine State Machine
        if (current_time >= next_audio_event_time) {
            if (audio_state == 0) {
                // Done playing current note, start pause/silence space before next note
                speaker_off();
                unsigned short duration = note_durations_ms[current_note_idx];
                unsigned short pause = note_pauses_ms[current_note_idx];
                next_audio_event_time = current_time + (pause - duration);
                audio_state = 1;
            } else {
                // Done with silent space, advance to next note and start playing
                current_note_idx = (current_note_idx + 1) % AUDIO_NOTES_COUNT;
                unsigned short freq = melody_freqs[current_note_idx];
                unsigned short duration = note_durations_ms[current_note_idx];
                
                if (freq > 0) {
                    speaker_on(freq);
                } else {
                    speaker_off();
                }
                
                next_audio_event_time = current_time + duration;
                audio_state = 0;
            }
        }
        
        // Video Engine State Machine
        if (current_time >= next_frame_time) {
            // Draw current frame and advance index
            draw_frame(current_frame_idx);
            current_frame_idx = (current_frame_idx + 1) % NYANCAT_FRAMES;
            
            // Standard Nyancat GIF frame duration is 83ms (~12 fps)
            next_frame_time = current_time + 83;
        }
        
        // Stall for 5 milliseconds to achieve a fine control granularity
        SystemTable->BootServices->Stall(5000);
        current_time += 5;
    }
    
    // 6. Safe Cleanup and shutdown
    speaker_off(); // Must ensure speaker is turned off!
    SystemTable->BootServices->FreePool(blt_buffer);
    
    // Reset and clear console to restore shell state
    SystemTable->ConOut->Reset(SystemTable->ConOut, BOOLEAN_FALSE);
    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    
    return EFI_SUCCESS;
}
