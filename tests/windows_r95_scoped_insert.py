#!/usr/bin/env python3
"""R95 opt-in, process-scoped Windows SendInput test.

This emits synthetic OS Insert presses, NOT physical hardware events. It is
ONLY for the separate disposable original Minecraft 1.8.9 launcher that has
already verified the target JVM PID, birth time, original JAR and Attach gates.
Never send a key unless the exact owned Minecraft window is foreground.
"""
import ctypes
from ctypes import wintypes
import os
from pathlib import Path
import re
import time

VK_INSERT = 0x2D
KEYEVENTF_KEYUP = 0x0002
INPUT_KEYBOARD = 1
ULONG_PTR = ctypes.c_uint64 if ctypes.sizeof(ctypes.c_void_p) == 8 else ctypes.c_uint32


class KEYBDINPUT(ctypes.Structure):
    _fields_ = [("wVk", wintypes.WORD),
                ("wScan", wintypes.WORD),
                ("dwFlags", wintypes.DWORD),
                ("time", wintypes.DWORD),
                ("dwExtraInfo", ULONG_PTR)]


class MOUSEINPUT(ctypes.Structure):
    _fields_ = [("dx", wintypes.LONG), ("dy", wintypes.LONG),
                ("mouseData", wintypes.DWORD), ("dwFlags", wintypes.DWORD),
                ("time", wintypes.DWORD), ("dwExtraInfo", ULONG_PTR)]


class HARDWAREINPUT(ctypes.Structure):
    _fields_ = [("uMsg", wintypes.DWORD), ("wParamL", wintypes.WORD),
                ("wParamH", wintypes.WORD)]


class INPUTUNION(ctypes.Union):
    _fields_ = [("ki", KEYBDINPUT), ("mi", MOUSEINPUT), ("hi", HARDWAREINPUT)]


class INPUT(ctypes.Structure):
    _fields_ = [("type", wintypes.DWORD), ("data", INPUTUNION)]


def check_offline_insert_collision(sandbox):
    """Fail closed on an explicit Minecraft 1.8.x options.txt Insert binding."""
    path = Path(sandbox) / "game" / "options.txt"
    if not path.is_file():
        raise RuntimeError("R95 sandbox options.txt unavailable")
    if path.stat().st_size > 1048576:
        raise RuntimeError("R95 sandbox options.txt unreasonably large")
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if line.startswith("key_"):
            name, sep, value = line.partition(":")
            if sep and re.fullmatch(r"\s*210\s*", value):
                raise RuntimeError("R95 Insert conflicts with Minecraft setting " + name)
    # Absence of file conflicts does not certify modded/runtime keybindings.


def send_isolated_insert(pid, sandbox, presses=2):
    """Only call inside the supervised *already-admitted* original client stage.

    Return the number of actual OS SendInput presses. SetForegroundWindow is
    attempted only for the exact target game window; if focus cannot be proven,
    no keypress is sent. Key-up is guaranteed for any issued key-down.
    """
    if os.name != "nt":
        raise RuntimeError("R95 SendInput is Windows-only")
    if not isinstance(pid, int) or not 1 <= pid <= 4294967295 or presses != 2:
        raise RuntimeError("R95 exact PID and exactly two synthetic presses required")
    check_offline_insert_collision(sandbox)
    user = ctypes.WinDLL("user32", use_last_error=True)
    user.EnumWindows.argtypes = [ctypes.c_void_p, wintypes.LPARAM]
    user.EnumWindows.restype = wintypes.BOOL
    user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    user.GetWindowThreadProcessId.restype = wintypes.DWORD
    user.IsWindowVisible.argtypes = [wintypes.HWND]
    user.IsWindowVisible.restype = wintypes.BOOL
    user.IsIconic.argtypes = [wintypes.HWND]
    user.IsIconic.restype = wintypes.BOOL
    user.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
    user.GetWindowTextW.restype = ctypes.c_int
    user.GetForegroundWindow.restype = wintypes.HWND
    user.SetForegroundWindow.argtypes = [wintypes.HWND]
    user.SetForegroundWindow.restype = wintypes.BOOL
    user.GetAsyncKeyState.argtypes = [ctypes.c_int]
    user.GetAsyncKeyState.restype = ctypes.c_short
    user.SendInput.argtypes = [wintypes.UINT, ctypes.POINTER(INPUT), ctypes.c_int]
    user.SendInput.restype = wintypes.UINT
    matches = []
    enumproc = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

    def inspect(hwnd, unused):
        owner = wintypes.DWORD()
        user.GetWindowThreadProcessId(hwnd, ctypes.byref(owner))
        if owner.value != pid or not user.IsWindowVisible(hwnd) or user.IsIconic(hwnd):
            return True
        title = ctypes.create_unicode_buffer(256)
        user.GetWindowTextW(hwnd, title, len(title))
        if title.value == "Minecraft 1.8.9":
            matches.append(hwnd)
        return True

    callback = enumproc(inspect)
    if not user.EnumWindows(callback, 0) or len(matches) != 1:
        raise RuntimeError("R95 exact original Minecraft window not uniquely identified")
    hwnd = matches[0]
    if user.GetForegroundWindow() != hwnd:
        user.SetForegroundWindow(hwnd)
        time.sleep(0.25)
    if user.GetForegroundWindow() != hwnd:
        raise RuntimeError("R95 owned game window is not foreground; no input sent")
    if user.GetAsyncKeyState(VK_INSERT) & 0x8000:
        raise RuntimeError("R95 Insert already down; no input sent")
    # The R94 JNI state requires an owner-focused released sample to arm.
    time.sleep(0.35)

    def key(down):
        value = INPUT(type=INPUT_KEYBOARD)
        value.data.ki = KEYBDINPUT(
            wVk=VK_INSERT, wScan=0,
            dwFlags=0 if down else KEYEVENTF_KEYUP,
            time=0, dwExtraInfo=0)
        if user.SendInput(1, ctypes.byref(value), ctypes.sizeof(INPUT)) != 1:
            raise RuntimeError("R95 SendInput refused")
    done = 0
    for _ in range(presses):
        if user.GetForegroundWindow() != hwnd:
            raise RuntimeError("R95 game lost foreground before Insert")
        try:
            key(True)
            time.sleep(0.23)
        finally:
            # Always send the paired release, even if focus changes.
            key(False)
        time.sleep(0.30)
        if user.GetForegroundWindow() != hwnd:
            raise RuntimeError("R95 game lost foreground during key test")
        done += 1
    if user.GetAsyncKeyState(VK_INSERT) & 0x8000:
        raise RuntimeError("R95 synthetic Insert remained pressed")
    return done
