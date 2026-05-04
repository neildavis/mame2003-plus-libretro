import tkinter as tk
from tkinter import ttk, filedialog
import sys
import json
import os

class PaletteApp:
    def __init__(self, root):
        self.root = root
        self.root.title("System 16B ROM Palette Patcher & Sprite Preview")
        self.root.configure(bg="#1e1e1e")

        self.BASE_ADDR = 0xE18C + (0xD20 - 0x800)
        self.PTR_BASE = 0xE18C
        
        self.color_list_1 = [
            0x000000, 0xf7f7f7, 0xb5d6c5, 0x94b5a4, 0x739484, 0x0073A4, 0x009CAD, 0x005BAD, 0x840010, 0xB55200, 0x000000, 0x940010, 0x840010, 0xB55200, 0x940010, 0x21f700,
            0x000000, 0xf7f7f7, 0xb5d6c5, 0x94b5a4, 0x739484, 0x0073A4, 0x009CAD, 0x005BAD, 0x840010, 0xB55200, 0x000000, 0x940010, 0xE68410, 0xF7C500, 0xF78410, 0x21f700,
            0x000000, 0xf7f7f7, 0xb5d6c5, 0x94b5a4, 0x739484, 0x0073A4, 0x009CAD, 0x005BAD, 0xE68410, 0xF7C500, 0x000000, 0xF78410, 0x840010, 0xB55200, 0x940010, 0x21f700,
            0x000000, 0xf7f7f7, 0xb5d6c5, 0x94b5a4, 0x739484, 0x00C5F7, 0x00EFFF, 0x00ADFF, 0x840010, 0xB55200, 0x000000, 0x940010, 0x840010, 0xB55200, 0x940010, 0x21f700
        ]
        
        self.color_list_2 = list(self.color_list_1)
        self.selected_idx = 0
        self.swatches_mod = []

        self.sprite_data = self.load_sprite_bin()
        self.preview_images = [tk.PhotoImage(width=160, height=256) for _ in range(4)]

        self.setup_styles()
        self.create_widgets()
        
        self.update_previews()
        self.refresh_c_code()
        self.select_swatch(0)

    def load_sprite_bin(self):
        if os.path.exists("sprite.bin"):
            with open("sprite.bin", "rb") as f: return f.read()
        return None

    def setup_styles(self):
        style = ttk.Style()
        style.theme_use('clam')
        style.configure("TFrame", background="#1e1e1e")
        style.configure("TLabel", background="#1e1e1e", foreground="#ffffff")
        style.configure("Ref.TLabel", background="#1e1e1e", foreground="#aaaaaa", font=("Consolas", 8))
        style.configure("Error.TLabel", background="#1e1e1e", foreground="#ff3333")
        style.configure("Reset.TButton", foreground="#ff6666")

    def create_widgets(self):
        main_container = ttk.Frame(self.root, padding="10")
        main_container.pack(fill=tk.BOTH, expand=True)

        ttk.Label(main_container, text="Original ROM Palette", font=("Arial", 10, "bold")).pack(anchor=tk.W)
        orig_frame = ttk.Frame(main_container); orig_frame.pack(pady=5)
        self.draw_grid(orig_frame, self.color_list_1, False)

        ttk.Label(main_container, text="Modified ROM Palette", font=("Arial", 10, "bold")).pack(anchor=tk.W, pady=(10,0))
        mod_frame = ttk.Frame(main_container); mod_frame.pack(pady=5)
        self.swatches_mod = self.draw_grid(mod_frame, self.color_list_2, True)

        edit_wrapper = ttk.Frame(main_container, padding="10"); edit_wrapper.pack(fill=tk.X, pady=5)
        self.r_val, self.g_val, self.b_val = tk.IntVar(), tk.IntVar(), tk.IntVar()
        self.dec_entry_vars = [tk.StringVar() for _ in range(3)]
        self.hex_entry_vars = [tk.StringVar() for _ in range(3)]
        self.ref_vars = [tk.StringVar() for _ in range(3)]
        
        slider_frame = ttk.Frame(edit_wrapper); slider_frame.pack(side=tk.LEFT, fill=tk.X, expand=True)
        self.sliders = [self.r_val, self.g_val, self.b_val]
        
        for i, (label_text, var, d_var, h_var, r_var) in enumerate(zip(["Red", "Green", "Blue"], self.sliders, self.dec_entry_vars, self.hex_entry_vars, self.ref_vars)):
            col_frame = ttk.Frame(slider_frame); col_frame.grid(row=0, column=i, padx=15)
            input_grid_frame = ttk.Frame(col_frame); input_grid_frame.pack()
            
            ttk.Label(input_grid_frame, text=f"{label_text}:", font=("Arial", 9, "bold")).grid(row=0, column=0, sticky=tk.W)
            ttk.Label(input_grid_frame, text="Orig:", style="Ref.TLabel").grid(row=1, column=0)
            ttk.Label(input_grid_frame, textvariable=r_var, style="Ref.TLabel").grid(row=1, column=1, sticky=tk.W)

            for row, (v, lbl) in enumerate([(d_var, "Dec:"), (h_var, "Hex:")]):
                ttk.Label(input_grid_frame, text=lbl, font=("Consolas", 8)).grid(row=row+2, column=0)
                ent = ttk.Entry(input_grid_frame, textvariable=v, width=5, font=("Consolas", 10))
                ent.grid(row=row+2, column=1, padx=2, pady=1)
                if lbl == "Dec:":
                    ent.bind("<Return>", lambda e, idx=i: self.on_dec_entry_submit(idx))
                else:
                    ent.bind("<Return>", lambda e, idx=i: self.on_hex_entry_submit(idx))

            ttk.Scale(col_frame, from_=0, to=255, variable=var, orient=tk.HORIZONTAL, command=self.on_slider_move).pack(pady=5)

        btn_frame = ttk.Frame(edit_wrapper); btn_frame.pack(side=tk.RIGHT, padx=10)
        self.btn_reset_color = ttk.Button(btn_frame, text="Reset Color", command=self.reset_single_color)
        self.btn_reset_color.pack(fill=tk.X, pady=2)
        ttk.Button(btn_frame, text="Reset Palette", command=self.reset_palette, style="Reset.TButton").pack(fill=tk.X, pady=2)

        ttk.Label(main_container, text="Sprite Previews", font=("Arial", 10, "bold")).pack(anchor=tk.W, pady=(15, 0))

        preview_frame = ttk.Frame(main_container); preview_frame.pack(fill=tk.X, pady=5)
        for i in range(4):
            container = ttk.Frame(preview_frame, padding=5); container.pack(side=tk.LEFT, expand=True)
            ttk.Label(container, text=f"Bank {i}", style="Ref.TLabel").pack()
            cvs = tk.Canvas(container, width=160, height=260, bg="#252525", highlightthickness=1)
            cvs.pack(); cvs.create_image(0, 0, anchor=tk.NW, image=self.preview_images[i])

        self.status_var = tk.StringVar()
        ttk.Label(main_container, textvariable=self.status_var, style="Error.TLabel").pack(anchor=tk.W)
        self.code_box = tk.Text(main_container, height=6, bg="#2d2d2d", fg="#00ff00", font=("Consolas", 10))
        self.code_box.pack(fill=tk.BOTH, expand=True, pady=10)
        
        actions = ttk.Frame(main_container); actions.pack(fill=tk.X, pady=5)
        ttk.Button(actions, text="Save Palette...", command=self.save_palette).pack(side=tk.LEFT, padx=2)
        ttk.Button(actions, text="Load Palette...", command=self.load_palette).pack(side=tk.LEFT, padx=2)
        ttk.Button(actions, text="Copy C-Code", command=self.copy_to_clip).pack(side=tk.RIGHT, padx=2)

    def draw_grid(self, parent, color_list, is_mod_grid):
        swatches = []
        for i, color in enumerate(color_list):
            canvas = tk.Canvas(parent, width=35, height=35, bg=f"#{color:06x}", highlightthickness=1, highlightbackground="#444444")
            canvas.grid(row=i//16, column=i%16, padx=1, pady=1)
            canvas.bind("<Button-1>", lambda e, idx=i: self.select_swatch(idx))
            if is_mod_grid:
                canvas.bind("<Button-3>", lambda e, idx=i: self.paste_from_selected(idx))
                swatches.append(canvas)
        return swatches

    def select_swatch(self, idx):
        self.selected_idx = idx
        for i, s in enumerate(self.swatches_mod):
            s.config(highlightbackground="#00ffff" if i == idx else "#444444", highlightthickness=2 if i == idx else 1)
        orig_c = self.color_list_1[idx]
        for i, val in enumerate([(orig_c >> 16) & 0xFF, (orig_c >> 8) & 0xFF, orig_c & 0xFF]):
            self.ref_vars[i].set(f"{val} / {val:02X}")
        color = self.color_list_2[idx]
        self.r_val.set((color >> 16) & 0xFF); self.g_val.set((color >> 8) & 0xFF); self.b_val.set(color & 0xFF)
        self.sync_entries_to_sliders(); self.check_button_states()

    def paste_from_selected(self, target_idx):
        source_color = self.color_list_2[self.selected_idx]
        self.color_list_2[target_idx] = source_color
        self.swatches_mod[target_idx].config(bg=f"#{source_color:06x}")
        self.update_previews()
        self.refresh_c_code()
        if target_idx == self.selected_idx:
            self.check_button_states()

    def sync_entries_to_sliders(self):
        for i in range(3):
            val = self.sliders[i].get()
            self.dec_entry_vars[i].set(str(val))
            self.hex_entry_vars[i].set(f"{val:02X}")

    def on_slider_move(self, _=None):
        self.sync_entries_to_sliders(); self.apply_color_update()

    def on_dec_entry_submit(self, idx):
        try:
            v = int(self.dec_entry_vars[idx].get())
            if 0 <= v <= 255: self.sliders[idx].set(v); self.apply_color_update()
        except: self.sync_entries_to_sliders()

    def on_hex_entry_submit(self, idx):
        try:
            v = int(self.hex_entry_vars[idx].get(), 16)
            if 0 <= v <= 255: self.sliders[idx].set(v); self.apply_color_update()
        except: self.sync_entries_to_sliders()

    def apply_color_update(self):
        r, g, b = [s.get() for s in self.sliders]
        new_c = (r << 16) | (g << 8) | b
        self.color_list_2[self.selected_idx] = new_c
        self.swatches_mod[self.selected_idx].config(bg=f"#{new_c:06x}")
        self.update_previews(); self.refresh_c_code(); self.check_button_states()
        self.sync_entries_to_sliders()

    def update_previews(self):
        if not self.sprite_data: return
        for bank_idx in range(4):
            img = self.preview_images[bank_idx]; img.blank()
            palette = self.color_list_2[bank_idx*16 : (bank_idx+1)*16]
            for y in range(64):
                ptr = 16 + (y * 16)
                if ptr + 16 > len(self.sprite_data): break
                x = 0
                for word_idx in range(0, 16, 2):
                    b1, b2 = self.sprite_data[ptr+word_idx], self.sprite_data[ptr+word_idx+1]
                    w = (b2 << 8) | b1
                    for p in [(w>>12)&0xF, (w>>8)&0xF, (w>>4)&0xF, w&0xF]:
                        if p != 0 and p != 0xF:
                            img.put(f"#{palette[p]:06x}", to=(16+(x*4), y*4, 16+((x+1)*4), (y+1)*4))
                        x += 1

    def convert_to_system16b(self, h24):
        r, g, b = (h24>>16&0xFF)>>3, (h24>>8&0xFF)>>3, (h24&0xFF)>>3
        val = ((r&0x1E)>>1) | ((r&0x01)<<12) | ((g&0x1E)<<3) | ((g&0x01)<<13) | ((b&0x1E)<<7) | ((b&0x01)<<14)
        return val

    def refresh_c_code(self):
        lines = ["void patch_rom_palette(UINT16 *ROM) {", "    UINT16 *paletterom16 = ROM + (0xe18c >> 1);"]
        changed = False
        for i, (c1, c2) in enumerate(zip(self.color_list_1, self.color_list_2)):
            if c1 != c2:
                changed = True
                addr = self.BASE_ADDR + (i * 2)
                lines.append(f"    paletterom16[0x{(addr - self.PTR_BASE)//2:04X}] = 0x{self.convert_to_system16b(c2):04X};")
        if not changed: lines.append("    // No changes")
        lines.append("}")
        self.code_box.delete("1.0", tk.END); self.code_box.insert(tk.END, "\n".join(lines))

    def check_button_states(self):
        state = 'disabled' if self.color_list_2[self.selected_idx] == self.color_list_1[self.selected_idx] else '!disabled'
        self.btn_reset_color.state([state])

    def reset_single_color(self):
        self.color_list_2[self.selected_idx] = self.color_list_1[self.selected_idx]
        self.select_swatch(self.selected_idx); self.apply_color_update()

    def reset_palette(self):
        self.color_list_2 = list(self.color_list_1)
        for i, c in enumerate(self.color_list_2): self.swatches_mod[i].config(bg=f"#{c:06x}")
        self.select_swatch(0); self.update_previews(); self.refresh_c_code()

    def save_palette(self):
        filename = filedialog.asksaveasfilename(
            defaultextension=".json",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")],
            title="Save Palette Data"
        )
        if filename:
            # Create a dictionary with keys palette_0 through palette_3
            grouped_palette = {}
            for bank_idx in range(4):
                bank_colors = self.color_list_2[bank_idx*16 : (bank_idx+1)*16]
                grouped_palette[f"palette_{bank_idx}"] = [f"0x{c:06X}" for c in bank_colors]
            
            with open(filename, 'w') as f:
                json.dump(grouped_palette, f, indent=4)
            self.status_var.set(f"Saved: {os.path.basename(filename)}")

    def load_palette(self):
        filename = filedialog.askopenfilename(
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")],
            title="Load Palette Data"
        )
        if filename and os.path.exists(filename):
            with open(filename, 'r') as f: 
                loaded_dict = json.load(f)
                
                # Reconstruct the flat list from palette_0...3
                new_list = []
                try:
                    for bank_idx in range(4):
                        key = f"palette_{bank_idx}"
                        if key in loaded_dict:
                            new_list.extend([int(c, 16) for c in loaded_dict[key]])
                        else:
                            # Fallback to original colors if a bank is missing in the JSON
                            new_list.extend(self.color_list_1[bank_idx*16 : (bank_idx+1)*16])
                    
                    self.color_list_2 = new_list[:64] # Ensure we don't exceed 64
                except (ValueError, KeyError, TypeError):
                    self.status_var.set("Error: Invalid JSON format.")
                    return

            for i, c in enumerate(self.color_list_2): 
                self.swatches_mod[i].config(bg=f"#{c:06x}")
            self.select_swatch(self.selected_idx)
            self.update_previews()
            self.refresh_c_code()
            self.status_var.set(f"Loaded: {os.path.basename(filename)}")

    def copy_to_clip(self):
        self.root.clipboard_clear(); self.root.clipboard_append(self.code_box.get("1.0", tk.END).strip())

if __name__ == "__main__":
    root = tk.Tk(); app = PaletteApp(root); root.mainloop()