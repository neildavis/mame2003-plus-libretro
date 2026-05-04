import os
from PIL import Image, ImageDraw
try:
    import pyperclip
    HAS_PYPERCLIP = True
except ImportError:
    HAS_PYPERCLIP = False

# --- INPUT DATA ---
color_list_1 = [
    0x000000, 0xf7f7f7, 0xb5d6c5, 0x94b5a4, 0x739484, 0x0073A4, 0x009CAD, 0x005BAD, 0x840010, 0xB55200, 0x000000, 0x940010, 0x840010, 0xB55200, 0x940010, 0x21f700,
    0x000000, 0xf7f7f7, 0xb5d6c5, 0x94b5a4, 0x739484, 0x0073A4, 0x009CAD, 0x005BAD, 0x840010, 0xB55200, 0x000000, 0x940010, 0xE68410, 0xF7C500, 0xF78410, 0x21f700,
    0x000000, 0xf7f7f7, 0xb5d6c5, 0x94b5a4, 0x739484, 0x0073A4, 0x009CAD, 0x005BAD, 0xE68410, 0xF7C500, 0x000000, 0xF78410, 0x840010, 0xB55200, 0x940010, 0x21f700,
    0x000000, 0xf7f7f7, 0xb5d6c5, 0x94b5a4, 0x739484, 0x00C5F7, 0x00EFFF, 0x00ADFF, 0x840010, 0xB55200, 0x000000, 0x940010, 0x840010, 0xB55200, 0x940010, 0x21f700
]

color_list_2 = [
    0x000000, 0xf7f7f7, 0xb5d6c5, 0x94b5a4, 0x739484, 0x00A473, 0x00AD9C, 0x00ADB5, 0x840010, 0xB55200, 0x000000, 0x940010, 0x840010, 0xB55200, 0x940010, 0x21f700,
    0x000000, 0xf7f7f7, 0xb5d6c5, 0x94b5a4, 0x739484, 0x00A473, 0x00AD9C, 0x00ADB5, 0x840010, 0xB55200, 0x000000, 0x940010, 0xE68410, 0xF7C500, 0xF78410, 0x21f700,
    0x000000, 0xf7f7f7, 0xb5d6c5, 0x94b5a4, 0x739484, 0x00A473, 0x00AD9C, 0x00ADB5, 0xE68410, 0xF7C500, 0x000000, 0xF78410, 0x840010, 0xB55200, 0x940010, 0x21f700,
    0x000000, 0xf7f7f7, 0xb5d6c5, 0x94b5a4, 0x739484, 0x00F7C5, 0x00FFEF, 0x00FFAD, 0x840010, 0xB55200, 0x000000, 0x940010, 0x840010, 0xB55200, 0x940010, 0x21f700
]

def convert_to_system16b(hex_24bit):
    r, g, b = (hex_24bit >> 16 & 0xFF) >> 3, (hex_24bit >> 8 & 0xFF) >> 3, (hex_24bit & 0xFF) >> 3
    val =  ((r & 0x1E) >> 1) | ((r & 0x01) << 12)
    val |= ((g & 0x1E) << 3) | ((g & 0x01) << 13)
    val |= ((b & 0x1E) << 7) | ((b & 0x01) << 14)
    return val

# --- CONFIG ---
# palette in ROM starts at 0xe18c - copied to palette RAM starting at 0xa00800
BASE_ADDR = 0xE18C + (0xD20 - 0x800) 
PTR_BASE = 0xE18C     
YELLOW, RESET = "\033[93m", "\033[0m"

# --- 1. CONSOLE LOG & C-CODE ---
print(f"{'Idx':<4} | {'ROM Addr':<10} | {'Original (Hex,S16)':<22} -> {'Modified (Hex,S16)':<22}")
print("-" * 85)

c_code_lines = [
    "void patch_rom_palette(UINT16 *ROM) {",
    "    UINT16 *paletterom16 = ROM + (0xe18c >> 1);"
]

for i, (c1, c2) in enumerate(zip(color_list_1, color_list_2)):
    addr = BASE_ADDR + (i * 2)
    s16_1 = convert_to_system16b(c1)
    s16_2 = convert_to_system16b(c2)
    
    line = f"{i:02d}   | 0x{addr:06X} | 0x{c1:06X}, 0x{s16_1:04X}      -> 0x{c2:06X}, 0x{s16_2:04X}"
    
    if c1 != c2:
        print(f"{YELLOW}{line}{RESET}")
        ptr_idx = (addr - PTR_BASE) // 2
        c_code_lines.append(f"    paletterom16[0x{ptr_idx:04X}] = 0x{s16_2:04X}; // ROM Addr: 0x{addr:06X}")
    else:
        print(line)

c_code_lines.append("}")
full_c_code = "\n".join(c_code_lines)
if HAS_PYPERCLIP:
    pyperclip.copy(full_c_code)

# --- 2. IMAGE RENDERING ---
def draw_palette_block(draw, colors, start_y, title):
    sw, pad, text_space = 80, 10, 55 
    draw.text((20, start_y), title, fill=(255, 255, 255))
    
    base_y = start_y + 40
    for i, hex_v in enumerate(colors):
        col = i % 16
        row = i // 16
        
        x = 20 + (col * (sw + pad))
        y = base_y + (row * (sw + text_space))
        
        rgb = ((hex_v >> 16) & 0xFF, (hex_v >> 8) & 0xFF, hex_v & 0xFF)
        draw.rectangle([x, y, x + sw, y + sw], fill=rgb, outline=(120, 120, 120))
        
        addr = BASE_ADDR + (i * 2)
        s16 = convert_to_system16b(hex_v)
        draw.text((x, y + sw + 5), f"ROM:0x{addr:06X}", fill=(0, 255, 255))
        draw.text((x, y + sw + 18), f"S16:0x{s16:04X}", fill=(255, 255, 0))
        draw.text((x, y + sw + 31), f"RGB:0x{hex_v:06X}", fill=(255, 255, 255))
    
    total_rows = ((len(colors) - 1) // 16) + 1
    return base_y + (total_rows * (sw + text_space)) + 40

rows_per_set = ((max(len(color_list_1), len(color_list_2)) - 1) // 16) + 1
row_height = 80 + 55 
calculated_height = (row_height * rows_per_set * 2) + 180 

img = Image.new("RGB", (1500, calculated_height), (30, 30, 30))
draw = ImageDraw.Draw(img)

next_y = draw_palette_block(draw, color_list_1, 20, "ORIGINAL ROM PALETTE")
draw_palette_block(draw, color_list_2, next_y, "MODIFIED ROM PALETTE")

img.save("palette_grid.png")

print(f"\n/* Generated C Code */\n{full_c_code}")
print(f"\nDone: C function updated. Image 'palette_grid.png' saved.")