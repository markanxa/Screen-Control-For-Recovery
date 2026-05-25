#!/usr/bin/env python3
import tkinter as tk
import subprocess
import time
import sys
import threading

# ====================== НАСТРОЙКИ ======================
def get_adb_prefix():
    try:
        subprocess.check_call(["adb", "shell", "su", "-c", "exit"],
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print("Режим: Root (su)")
        return ["adb", "shell", "su", "-c"], "/data/local/tmp"
    except:
        print("Режим: Стандартный (без su)")
        return ["adb", "shell"], "/tmp"

cmd_prefix, bin_path = get_adb_prefix()

PHONE_W, PHONE_H = 1080, 2460
skip = 4

# ====================== КАЛИБРОВКА (меняй здесь) ======================
OFFSET_X = 0
OFFSET_Y = 35          # ← было 40, уменьшил на 5 пикселей
SCALE_Y = 0.985        # ← чуть меньше 1.0, чтобы компенсировать смещение вниз

WINDOW_SCALE = 0.65    # масштаб окна при запуске (0.6 - 0.8)

print(f"Калибровка: OFFSET_X={OFFSET_X}, OFFSET_Y={OFFSET_Y}, SCALE_Y={SCALE_Y}")

# ====================== ЗАПУСК ======================
touch_proc = subprocess.Popen(cmd_prefix + [f"{bin_path}/virt_touch"], stdin=subprocess.PIPE, text=True)
key_proc = subprocess.Popen(cmd_prefix + [f"{bin_path}/virt_key"], stdin=subprocess.PIPE, text=True)

shot_proc = subprocess.Popen(
    cmd_prefix + [f"{bin_path}/shot_raw", str(skip)],
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE # <--- Возвращаем PIPE
)

# 2. Обновленная функция чтения
def read_stderr():
    global ow, oh
    while True:
        line = shot_proc.stderr.readline()
        if not line: break
        line = line.decode('utf-8', errors='ignore').strip()

        # Печатаем всё, что приходит из shot_raw, в ваш терминал
        print(line)

        if "FFPLAY SIZE:" in line:
            try:
                ow, oh = map(int, line.split("FFPLAY SIZE:")[-1].strip().split('x'))
            except:
                pass

threading.Thread(target=read_stderr, daemon=True).start()
time.sleep(2)

if not ow or not oh:
    print("❌ Не удалось получить размеры")
    sys.exit(1)

ffplay_proc = subprocess.Popen([
    "ffplay",
    "-f", "rawvideo",
    "-loglevel", "quiet",
    "-pixel_format", "rgb24",
    "-video_size", f"{ow}x{oh}",
    "-framerate", "15",
    "-"
], stdin=shot_proc.stdout)

# ====================== GUI ======================
root = tk.Tk()
root.title("ADB Remote Control")

init_w = int(ow * WINDOW_SCALE)
init_h = int(oh * WINDOW_SCALE)
root.geometry(f"{init_w}x{init_h}")
root.resizable(True, True)

canvas = tk.Canvas(root, width=init_w, height=init_h, bg="#000000", highlightthickness=0)
canvas.pack(fill="both", expand=True)

# ====================== СИНХРОНИЗАЦИЯ РАЗМЕРОВ ======================
last_w, last_h = init_w, init_h

def sync_ffplay_size(event=None):
    global last_w, last_h
    new_w = root.winfo_width()
    new_h = root.winfo_height()
    if new_w == last_w and new_h == last_h:
        return
    last_w, last_h = new_w, new_h
    canvas.config(width=new_w, height=new_h)
    try:
        subprocess.run(["xdotool", "search", "--name", "ffplay",
                        "windowsize", "%", str(new_w), str(new_h)],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    except:
        pass

root.bind("<Configure>", sync_ffplay_size)

# ====================== ТАЧ ======================
def on_click(event):
    if not ow or not oh: return

    x_ratio = event.x / canvas.winfo_width()
    y_ratio = event.y / canvas.winfo_height()

    x_phone = int(x_ratio * PHONE_W + OFFSET_X)
    y_phone = int(y_ratio * PHONE_H * SCALE_Y + OFFSET_Y)

    x_phone = max(0, min(x_phone, PHONE_W - 1))
    y_phone = max(0, min(y_phone, PHONE_H - 1))

    print(f"Click: win({event.x},{event.y}) → phone({x_phone},{y_phone}) | ratio_y={y_ratio:.3f}")

    touch_proc.stdin.write(f"{x_phone} {y_phone}\n")
    touch_proc.stdin.flush()

# ====================== КЛАВИАТУРА ======================
CHARS_MAP = {
    'a':30,'b':48,'c':46,'d':32,'e':18,'f':33,'g':34,'h':35,'i':23,'j':36,
    'k':37,'l':38,'m':50,'n':49,'o':24,'p':25,'q':16,'r':19,'s':31,'t':20,
    'u':22,'v':47,'w':17,'x':45,'y':21,'z':44,
    '1':2,'2':3,'3':4,'4':5,'5':6,'6':7,'7':8,'8':9,'9':10,'0':11,
    '-':12,'=':13,'.':52,'/':53,',':51,' ':57,'\n':28,'\t':15
}

KEY_MAP = {
    'Return':28,'BackSpace':14,'space':57,'Escape':1,
    'Up':103,'Down':108,'Left':105,'Right':106,'Tab':15
}

def send_key(code, state):
    if key_proc.poll() is None:
        key_proc.stdin.write(f"{code} {state}\n")
        key_proc.stdin.flush()

def on_key(event, state):
    if event.state & 0x4 and event.keysym.lower() == 'v' and state == 1:
        try:
            text = root.clipboard_get()
            for char in text:
                low = char.lower()
                if low in CHARS_MAP:
                    code = CHARS_MAP[low]
                    needs_shift = char.isupper() or char in '!@#$%^&*()_+'
                    if needs_shift: send_key(42, 1)
                    send_key(code, 1)
                    send_key(code, 0)
                    if needs_shift: send_key(42, 0)
                    time.sleep(0.01)
        except:
            pass
        return

    key_name = event.keysym
    if len(key_name) == 1 and key_name.lower() in CHARS_MAP:
        send_key(CHARS_MAP[key_name.lower()], state)
    elif key_name in KEY_MAP:
        send_key(KEY_MAP[key_name], state)

canvas.bind("<Button-1>", on_click)
root.bind("<KeyPress>", lambda e: on_key(e, 1))
root.bind("<KeyRelease>", lambda e: on_key(e, 0))

root.focus_force()
canvas.focus_set()

print("🎮 Готово! Меняй OFFSET_Y и SCALE_Y в начале кода.")
root.mainloop()
