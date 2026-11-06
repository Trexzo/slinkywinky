#!/usr/bin/env python3
"""R102: read-only exact-PID original Minecraft window identity evidence.

This is deliberately external to the game's JNI / WGL owner audit.
It does not inject, focus, click, type, subclass, hook or read game memory.
A PID + Win32 window title alone does NOT prove game-render/HDC identity.
"""
import ctypes
from ctypes import wintypes
import os
import time


EXPECTED_TITLE = "Minecraft 1.8.9"
MIN_WIDTH, MAX_WIDTH = 320, 8192
MIN_HEIGHT, MAX_HEIGHT = 240, 8192


class WindowAuditError(RuntimeError):
    pass


def classify_snapshot(items, expected_pid, expected_birth):
    """Pure validation suitable for cross-platform negative-control tests."""
    if not isinstance(expected_pid, int) or expected_pid <= 0 or not expected_birth:
        raise WindowAuditError("Missing verified original process PID/birth")
    if not isinstance(items, (list, tuple)):
        raise WindowAuditError("Window evidence is not a sequence")
    candidates = []
    for x in items:
        if x.get("pid") != expected_pid or x.get("title") != EXPECTED_TITLE:
            continue
        if x.get("birth") != expected_birth:
            raise WindowAuditError("Same PID with different kernel creation time")
        if x.get("visible") and not x.get("minimized"):
            candidates.append(x)
    if len(candidates) != 1:
        raise WindowAuditError("Original game title does not resolve to exactly one visible window")
    item = candidates[0]
    width, height = item.get("width"), item.get("height")
    if (not isinstance(width, int) or not isinstance(height, int) or
        not MIN_WIDTH <= width <= MAX_WIDTH or
        not MIN_HEIGHT <= height <= MAX_HEIGHT or
        item.get("enabled") is not True or
        item.get("root") is not True or
        not isinstance(item.get("thread_id"), int) or
        item["thread_id"] <= 0):
        raise WindowAuditError("Original game window has invalid geometry/ownership")
    return {
        "pid": expected_pid,
        "process_birth": str(expected_birth),
        "window_count": 1,
        "width": width,
        "height": height,
        "foreground": item.get("foreground") is True,
        "background": item.get("foreground") is False,
        "identity": "exact-pid-birth-title-visible-root",
        "window_render_hdc_certified": False,
        "input_capture_certified": False,
    }


def win32_window_snapshot(pid, birth):
    """Enumerate relevant windows without changing the foreground window."""
    if os.name != "nt":
        raise WindowAuditError("Win32-only original window inspection")
    user = ctypes.WinDLL("user32", use_last_error=True)
    user.EnumWindows.argtypes = [ctypes.c_void_p, wintypes.LPARAM]
    user.EnumWindows.restype = wintypes.BOOL
    user.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
    user.GetWindowThreadProcessId.restype = wintypes.DWORD
    user.GetWindowTextW.argtypes = [wintypes.HWND, wintypes.LPWSTR, ctypes.c_int]
    user.GetWindowTextW.restype = ctypes.c_int
    user.GetClientRect.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.RECT)]
    user.GetClientRect.restype = wintypes.BOOL
    user.GetForegroundWindow.restype = wintypes.HWND
    user.IsWindowVisible.argtypes = [wintypes.HWND]
    user.IsWindowVisible.restype = wintypes.BOOL
    user.IsWindowEnabled.argtypes = [wintypes.HWND]
    user.IsWindowEnabled.restype = wintypes.BOOL
    user.IsIconic.argtypes = [wintypes.HWND]
    user.IsIconic.restype = wintypes.BOOL
    user.GetAncestor.argtypes = [wintypes.HWND, wintypes.UINT]
    user.GetAncestor.restype = wintypes.HWND
    ga_root = 2
    foreground = user.GetForegroundWindow()
    windows = []
    callback_type = ctypes.WINFUNCTYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

    def enum_window(hwnd, _):
        owner = wintypes.DWORD()
        thread = int(user.GetWindowThreadProcessId(hwnd, ctypes.byref(owner)))
        if owner.value != pid:
            return True
        title_buffer = ctypes.create_unicode_buffer(129)
        user.GetWindowTextW(hwnd, title_buffer, 129)
        if title_buffer.value != EXPECTED_TITLE:
            return True
        rect = wintypes.RECT()
        geometry_ok = bool(user.GetClientRect(hwnd, ctypes.byref(rect)))
        windows.append({
            "pid": owner.value,
            "birth": birth,
            "title": title_buffer.value,
            "visible": bool(user.IsWindowVisible(hwnd)),
            "minimized": bool(user.IsIconic(hwnd)),
            "enabled": bool(user.IsWindowEnabled(hwnd)),
            "root": int(user.GetAncestor(hwnd, ga_root)) == int(hwnd),
            "thread_id": thread,
            "width": int(rect.right - rect.left) if geometry_ok else -1,
            "height": int(rect.bottom - rect.top) if geometry_ok else -1,
            "foreground": int(foreground) == int(hwnd),
        })
        return True

    callback = callback_type(enum_window)
    if not user.EnumWindows(callback, 0):
        raise WindowAuditError("Win32 EnumWindows did not complete")
    return windows


def observe_owned_game(pid, birth, samples=8, interval=0.125):
    """Observe one exact window repeatedly; no deliberate focus transitions."""
    if samples != 8 or not (0 <= interval <= 1):
        raise ValueError("R102 requires exactly eight bounded passive samples")
    first = None
    seen = []
    for i in range(samples):
        sample = classify_snapshot(win32_window_snapshot(pid, birth), pid, birth)
        signature = (sample["width"], sample["height"], sample["identity"])
        if first is None:
            first = signature
        elif signature != first:
            raise WindowAuditError("Original game window identity/dimensions changed mid-sampling")
        seen.append("foreground" if sample["foreground"] else "background")
        if i < samples-1:
            time.sleep(interval)
    return {
        "stage": "R102_READ_ONLY_ORIGINAL_GAME_WINDOW",
        "samples": samples,
        "foreground_samples": seen.count("foreground"),
        "background_samples": seen.count("background"),
        "refused_samples": 0,
        "window": {
            "pid": pid,
            "birth_token_recorded": True,
            "width": first[0],
            "height": first[1],
            "identity": first[2],
        },
        "read_only": True,
        "foreground_forced": False,
        "os_input_sent": False,
        "render_hdc_certified": False,
        "input_capture_certified": False,
    }
