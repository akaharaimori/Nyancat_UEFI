import PIL.Image
import io
import urllib.request

def compress_gif(header_path):
    # Direct URL to the GIF in TakWolf's NyanCatEverywhere repository
    url = "https://raw.githubusercontent.com/TakWolf/NyanCatEverywhere/master/www/img/nyancat.gif"
    print(f"Downloading Nyancat GIF into memory from {url}...")
    try:
        req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
        gif_bytes = urllib.request.urlopen(req).read()
    except Exception as e:
        print(f"Error downloading GIF: {e}")
        return False

    img = PIL.Image.open(io.BytesIO(gif_bytes))
    frames = []
    
    # Background color: #003366 -> RGB (0, 51, 102) -> BGRA in x86: Blue=102, Green=51, Red=0, Alpha=0
    # In hex: 0x00663300
    bg_color = (0, 51, 102, 255)
    
    width, height = img.size
    print(f"GIF size: {width}x{height}, Frames: {img.n_frames}")
    
    for frame_idx in range(img.n_frames):
        img.seek(frame_idx)
        frame_rgba = img.convert('RGBA')
        pixels = list(frame_rgba.getdata())
        
        # Apply RLE
        runs = []
        current_color = None
        current_count = 0
        
        for p in pixels:
            # If transparent (A == 0), replace with background color
            if p[3] == 0:
                color_rgb = bg_color
            else:
                color_rgb = p
                
            # Convert to 32-bit BGRA (stored as little endian: Blue, Green, Red, Reserved)
            r, g, b, _ = color_rgb
            color_u32 = (r << 16) | (g << 8) | b
            
            if current_color is None:
                current_color = color_u32
                current_count = 1
            elif color_u32 == current_color:
                current_count += 1
                if current_count == 65535: # limit to 16-bit count
                    runs.append((current_count, current_color))
                    current_count = 0
                    current_color = None
            else:
                runs.append((current_count, current_color))
                current_color = color_u32
                current_count = 1
                
        if current_count > 0:
            runs.append((current_count, current_color))
            
        frames.append(runs)
        print(f"Frame {frame_idx}: runs={len(runs)} (original pixels={len(pixels)}, compression={len(runs)/len(pixels)*100:.2f}%)")
        
    # Write to C header
    with open(header_path, 'w', encoding='utf-8') as f:
        f.write("/* Auto-generated Nyancat frames header */\n")
        f.write("#ifndef NYANCAT_FRAMES_H\n")
        f.write("#define NYANCAT_FRAMES_H\n\n")
        
        f.write(f"#define NYANCAT_WIDTH {width}\n")
        f.write(f"#define NYANCAT_HEIGHT {height}\n")
        f.write(f"#define NYANCAT_FRAMES {len(frames)}\n\n")
        
        f.write("typedef struct {\n")
        f.write("    unsigned short count;\n")
        f.write("    unsigned int color;\n")
        f.write("} RleRun;\n\n")
        
        for idx, runs in enumerate(frames):
            f.write(f"const RleRun frame_{idx}[] = {{\n")
            for count, color in runs:
                f.write(f"    {{{count}, 0x{color:08X}}},\n")
            f.write("};\n\n")
            
        f.write("typedef struct {\n")
        f.write("    const RleRun *runs;\n")
        f.write("    unsigned int num_runs;\n")
        f.write("} RleFrame;\n\n")
        
        f.write("const RleFrame nyancat_frames[] = {\n")
        for idx, runs in enumerate(frames):
            f.write(f"    {{frame_{idx}, {len(runs)}}},\n")
        f.write("};\n\n")
        
        f.write("#endif /* NYANCAT_FRAMES_H */\n")
        
    print(f"Header written to {header_path}")
    return True

if __name__ == '__main__':
    out = "nyancat_frames.h"
    compress_gif(out)
