import re
import urllib.request

def parse_audio(header_path):
    base_url = "https://raw.githubusercontent.com/electricmango/Arduino-Music-Project/master/Nyan_Cat/"
    
    print(f"Downloading pitches.h into memory...")
    try:
        req_pitches = urllib.request.Request(base_url + "pitches.h", headers={'User-Agent': 'Mozilla/5.0'})
        pitches_text = urllib.request.urlopen(req_pitches).read().decode('utf-8')
    except Exception as e:
        print(f"Error downloading pitches.h: {e}")
        return False
        
    print(f"Downloading Nyan_Cat.ino into memory...")
    try:
        req_ino = urllib.request.Request(base_url + "Nyan_Cat.ino", headers={'User-Agent': 'Mozilla/5.0'})
        ino_text = urllib.request.urlopen(req_ino).read().decode('utf-8')
    except Exception as e:
        print(f"Error downloading Nyan_Cat.ino: {e}")
        return False

    # Parse pitches
    pitch_map = {'0': 0} # 0 is rest
    for line in pitches_text.splitlines():
        m = re.match(r'#define\s+(NOTE_\w+)\s+(\d+)', line)
        if m:
            pitch_map[m.group(1)] = int(m.group(2))
                
    # Find melody array
    melody_match = re.search(r'int\s+melody\s*\[\s*\]\s*=\s*\{([^}]+)\}', ino_text, re.DOTALL)
    if not melody_match:
        raise ValueError("Could not find melody array in INO")
    melody_raw = melody_match.group(1)
    melody_notes = [n.strip() for n in re.split(r',', melody_raw) if n.strip()]
    
    # Find noteDurations array
    durations_match = re.search(r'int\s+noteDurations\s*\[\s*\]\s*=\s*\{([^}]+)\}', ino_text, re.DOTALL)
    if not durations_match:
        raise ValueError("Could not find noteDurations array in INO")
    durations_raw = durations_match.group(1)
    durations_vals = [d.strip() for d in re.split(r',', durations_raw) if d.strip()]
    
    print(f"Parsed {len(melody_notes)} notes and {len(durations_vals)} durations")
    
    if len(melody_notes) != len(durations_vals):
        print("Warning: melody and durations count mismatch!")
        
    # Generate C header
    with open(header_path, 'w', encoding='utf-8') as f:
        f.write("/* Auto-generated Nyancat audio header */\n")
        f.write("#ifndef NYANCAT_AUDIO_H\n")
        f.write("#define NYANCAT_AUDIO_H\n\n")
        
        f.write(f"#define AUDIO_NOTES_COUNT {len(melody_notes)}\n\n")
        
        f.write("const unsigned short melody_freqs[] = {\n")
        for i, note in enumerate(melody_notes):
            freq = pitch_map.get(note, None)
            if freq is None:
                try:
                    freq = int(note)
                except ValueError:
                    raise ValueError(f"Unknown note '{note}' in melody at index {i}")
            f.write(f"    {freq}, /* {note} */\n")
        f.write("};\n\n")
        
        f.write("const unsigned short note_durations_ms[] = {\n")
        for i, dur_str in enumerate(durations_vals):
            dur = int(dur_str)
            note_duration_ms = 1000 // dur
            f.write(f"    {note_duration_ms}, /* {dur} */\n")
        f.write("};\n\n")
        
        f.write("const unsigned short note_pauses_ms[] = {\n")
        for i, dur_str in enumerate(durations_vals):
            dur = int(dur_str)
            note_duration_ms = 1000 // dur
            pause_ms = int(note_duration_ms * 1.30)
            f.write(f"    {pause_ms},\n")
        f.write("};\n\n")
        
        f.write("#endif /* NYANCAT_AUDIO_H */\n")
        
    print(f"Audio header written to {header_path}")
    return True

if __name__ == '__main__':
    out = "nyancat_audio.h"
    parse_audio(out)
