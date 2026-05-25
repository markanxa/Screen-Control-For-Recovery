## Screen Control For Recovery — Project Overview

**Screen Control For Recovery** is a lightweight tool for remote control of Android devices in Recovery mode.

Unlike standard solutions (such as scrcpy), this project operates at a low level within the Linux kernel, bypassing Android's standard input/output limitations. For minimal-latency screen capture, it uses the **DRM (Direct Rendering Manager)** subsystem, while touch and key emulation is implemented via the **uinput** kernel interface.

---

### Key Features

- **Minimal Latency (Zero-copy):** Direct reading of GPU framebuffer memory via `/dev/dri/card0` using the `mmap` system call.
- **Efficiency:** Streams raw pixel data directly to `ffplay` on the PC, avoiding unnecessary encoding.
- **Autonomy:** The tool does not require a fully booted Android system (no SurfaceFlinger, Gralloc, etc.) and works in an isolated Recovery environment.

---

### Component Overview

| **Component** | **Description** |
|---------------|-----------------|
| `pc_controller.py` | PC-side control script. Provides a graphical interface, coordinate calibration, and transmits mouse/keyboard actions. |
| `shot_raw` | Screen capture module. Reads data directly from the GPU framebuffer. |
| `virt_touch`, `virt_key` | Modules for emulating touch and keyboard input via `/dev/uinput`. |

---

### Setup Instructions

1. **Transfer Binaries to Device**

   Use ADB to push the binaries to the device and set execution permissions. See `adbpush.txt` for details:

   ```bash
   adb push shot_raw /sbin/
   adb push virt_touch /sbin/
   adb push virt_key /sbin/

   adb shell chmod +x /sbin/shot_raw
   adb shell chmod +x /sbin/virt_touch
   adb shell chmod +x /sbin/virt_key
   ```

2. **Launch Control**

   Ensure the device is connected via ADB and in Recovery mode, then run the PC script:

   ```bash
   python pc_controller.py
   ```

---

### Calibration and Configuration

- **Coordinate Calibration:**  
  Screen geometry and digitizer configurations vary by device. If mouse clicks are misaligned:
  - Open `pc_controller.py` in a text editor.
  - Adjust `OFFSET_Y`, `SCALE_Y`, and optionally `-framerate` (default: 15 to reduce USB load).
  - Restart the PC script to apply changes.

- **Permissions:**  
  The utilities require access to `/dev/dri/` and `/dev/uinput`. These are typically available with root privileges in custom Recovery environments.

- **Compatibility:**  
  DRM availability depends on the Linux kernel configuration and video system initialization in Recovery for each device.

---

## Экранное управление для режима Recovery — Обзор проекта

**Screen Control For Recovery** — это легковесный инструмент для удалённого управления Android-устройствами, находящимися в режиме Recovery.

В отличие от стандартных решений (например, scrcpy), этот проект работает на низком уровне ядра Linux, обходя ограничения стандартного ввода/вывода Android. Для захвата экрана с минимальной задержкой используется подсистема **DRM (Direct Rendering Manager)**, а эмуляция нажатий и жестов реализована через интерфейс **uinput** на уровне ядра.

---

### Ключевые особенности

- **Минимальная задержка (Zero-copy):** Прямое чтение видеопамяти графического процессора через `/dev/dri/card0` с использованием системного вызова `mmap`.
- **Эффективность:** Потоковая передача «сырых» пикселей напрямую в `ffplay` на ПК без лишнего кодирования.
- **Автономность:** Инструмент не требует наличия полностью запущенной системы Android (нет активных служб SurfaceFlinger, Gralloc и т.д.) и работает в изолированной среде Recovery.

---

### Описание компонентов

| **Компонент** | **Описание** |
|---------------|-------------|
| `pc_controller.py` | Скрипт управления для ПК. Обеспечивает графический интерфейс, калибровку координат и трансляцию действий мыши/клавиатуры. |
| `shot_raw` | Модуль захвата экрана. Считывает данные напрямую из кадрового буфера графического чипа. |
| `virt_touch`, `virt_key` | Модули для эмуляции сенсорного ввода и клавиатуры через `/dev/uinput`. |

---

### Инструкция по подготовке

1. **Перенос бинарных файлов на устройство**

   Используйте ADB для переноса файлов и предоставления прав на исполнение:

   ```bash
   adb push shot_raw /sbin/
   adb push virt_touch /sbin/
   adb push virt_key /sbin/

   adb shell chmod +x /sbin/shot_raw
   adb shell chmod +x /sbin/virt_touch
   adb shell chmod +x /sbin/virt_key
   ```

2. **Запуск управления**

   Убедитесь, что устройство подключено по ADB и находится в режиме Recovery, после чего запустите скрипт на ПК:

   ```bash
   python pc_controller.py
   ```

---

### Калибровка и настройка

- **Калибровка координат:**
  Геометрия экранов и конфигурации дигитайзеров различаются в зависимости от модели устройства. Если нажатия мыши смещены:
  - Откройте `pc_controller.py` в текстовом редакторе.
  - Отрегулируйте параметры `OFFSET_Y`, `SCALE_Y` и, при необходимости, `-framerate` (по умолчанию 15, чтобы снизить нагрузку на USB-кабель).
  - Перезапустите скрипт управления на ПК для проверки изменений.

- **Права доступа:**
  Работа утилит зависит от доступа к `/dev/dri/` и `/dev/uinput`. Обычно эти устройства доступны с правами суперпользователя (root) в кастомных Recovery.

- **Совместимость:**
  Доступность DRM напрямую зависит от конфигурации ядра Linux и инициализации видеосистемы при старте Recovery на конкретном устройстве.
