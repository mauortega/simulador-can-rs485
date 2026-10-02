"""Icone na bandeja do Windows, sem pystray nem Pillow.

No Linux as funcoes existem e nao fazem nada, para o mesmo codigo abrir a janela.
"""

from __future__ import annotations

import ctypes
import os
import queue
import threading
import time
from ctypes import wintypes

WM_APP = 0x8000
WM_LBUTTONUP = 0x0202
WM_LBUTTONDBLCLK = 0x0203
WM_RBUTTONUP = 0x0205
WM_DESTROY = 0x0002
WM_NULL = 0x0000
WM_QUIT = 0x0012
NIM_ADD = 0x00000000
NIM_MODIFY = 0x00000001
NIM_DELETE = 0x00000002
NIF_MESSAGE = 0x00000001
NIF_ICON = 0x00000002
NIF_TIP = 0x00000004
MF_STRING = 0x00000000
TPM_RIGHTALIGN = 0x0008
TPM_RETURNCMD = 0x0100
TPM_NONOTIFY = 0x0080
PM_REMOVE = 0x0001
IDI_APPLICATION = 32512
ID_ABRIR = 1
ID_SAIR = 2

# WINFUNCTYPE so existe no Python de Windows. No Linux o modulo importa,
# mas a bandeja fica desligada.
if os.name == "nt":
    LRESULT = ctypes.c_ssize_t
    WNDPROC = ctypes.WINFUNCTYPE(LRESULT, wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM)

    class WNDCLASSW(ctypes.Structure):
        _fields_ = [
            ("style", wintypes.UINT),
            ("lpfnWndProc", WNDPROC),
            ("cbClsExtra", ctypes.c_int),
            ("cbWndExtra", ctypes.c_int),
            ("hInstance", wintypes.HINSTANCE),
            ("hIcon", wintypes.HICON),
            ("hCursor", wintypes.HCURSOR),
            ("hbrBackground", wintypes.HBRUSH),
            ("lpszMenuName", wintypes.LPCWSTR),
            ("lpszClassName", wintypes.LPCWSTR),
        ]

    class NOTIFYICONDATAW(ctypes.Structure):
        _fields_ = [
            ("cbSize", wintypes.DWORD),
            ("hWnd", wintypes.HWND),
            ("uID", wintypes.UINT),
            ("uFlags", wintypes.UINT),
            ("uCallbackMessage", wintypes.UINT),
            ("hIcon", wintypes.HICON),
            ("szTip", wintypes.WCHAR * 128),
            ("dwState", wintypes.DWORD),
            ("dwStateMask", wintypes.DWORD),
            ("szInfo", wintypes.WCHAR * 256),
            ("uTimeout", wintypes.UINT),
            ("szInfoTitle", wintypes.WCHAR * 64),
            ("dwInfoFlags", wintypes.DWORD),
            ("guidItem", ctypes.c_byte * 16),
            ("hBalloonIcon", wintypes.HICON),
        ]


def disponivel() -> bool:
    return os.name == "nt"


def deve_ocultar_na_bandeja(tem_bandeja: bool, estado_janela: str, ja_oculta: bool) -> bool:
    return tem_bandeja and not ja_oculta and estado_janela == "iconic"


_api_ok = False
user32 = None
shell32 = None
kernel32 = None


def _garantir_api() -> None:
    global _api_ok, user32, shell32, kernel32
    if _api_ok:
        return
    user32 = ctypes.WinDLL("user32", use_last_error=True)
    shell32 = ctypes.WinDLL("shell32", use_last_error=True)
    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

    user32.RegisterClassW.argtypes = [ctypes.POINTER(WNDCLASSW)]
    user32.RegisterClassW.restype = wintypes.ATOM
    user32.CreateWindowExW.argtypes = [
        wintypes.DWORD,
        wintypes.LPCWSTR,
        wintypes.LPCWSTR,
        wintypes.DWORD,
        ctypes.c_int,
        ctypes.c_int,
        ctypes.c_int,
        ctypes.c_int,
        wintypes.HWND,
        wintypes.HMENU,
        wintypes.HINSTANCE,
        wintypes.LPVOID,
    ]
    user32.CreateWindowExW.restype = wintypes.HWND
    user32.DefWindowProcW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
    user32.DefWindowProcW.restype = LRESULT
    user32.LoadIconW.argtypes = [wintypes.HINSTANCE, wintypes.LPCVOID]
    user32.LoadIconW.restype = wintypes.HICON
    user32.DestroyWindow.argtypes = [wintypes.HWND]
    user32.DestroyWindow.restype = wintypes.BOOL
    user32.PeekMessageW.argtypes = [
        ctypes.POINTER(wintypes.MSG),
        wintypes.HWND,
        wintypes.UINT,
        wintypes.UINT,
        wintypes.UINT,
    ]
    user32.PeekMessageW.restype = wintypes.BOOL
    user32.TranslateMessage.argtypes = [ctypes.POINTER(wintypes.MSG)]
    user32.TranslateMessage.restype = wintypes.BOOL
    user32.DispatchMessageW.argtypes = [ctypes.POINTER(wintypes.MSG)]
    user32.DispatchMessageW.restype = LRESULT
    user32.PostQuitMessage.argtypes = [ctypes.c_int]
    user32.CreatePopupMenu.argtypes = []
    user32.CreatePopupMenu.restype = wintypes.HMENU
    user32.AppendMenuW.argtypes = [wintypes.HMENU, wintypes.UINT, ctypes.c_size_t, wintypes.LPCWSTR]
    user32.AppendMenuW.restype = wintypes.BOOL
    user32.TrackPopupMenu.argtypes = [
        wintypes.HMENU,
        wintypes.UINT,
        ctypes.c_int,
        ctypes.c_int,
        ctypes.c_int,
        wintypes.HWND,
        ctypes.c_void_p,
    ]
    user32.TrackPopupMenu.restype = ctypes.c_uint
    user32.DestroyMenu.argtypes = [wintypes.HMENU]
    user32.DestroyMenu.restype = wintypes.BOOL
    user32.GetCursorPos.argtypes = [ctypes.POINTER(wintypes.POINT)]
    user32.GetCursorPos.restype = wintypes.BOOL
    user32.SetForegroundWindow.argtypes = [wintypes.HWND]
    user32.SetForegroundWindow.restype = wintypes.BOOL
    user32.PostMessageW.argtypes = [wintypes.HWND, wintypes.UINT, wintypes.WPARAM, wintypes.LPARAM]
    user32.PostMessageW.restype = wintypes.BOOL
    kernel32.GetModuleHandleW.argtypes = [wintypes.LPCWSTR]
    kernel32.GetModuleHandleW.restype = wintypes.HINSTANCE
    shell32.Shell_NotifyIconW.argtypes = [wintypes.DWORD, ctypes.POINTER(NOTIFYICONDATAW)]
    shell32.Shell_NotifyIconW.restype = wintypes.BOOL
    _api_ok = True


class Bandeja:
    def __init__(self, ao_restaurar, ao_sair):
        self._ao_restaurar = ao_restaurar
        self._ao_sair = ao_sair
        self._fila: queue.Queue = queue.Queue()
        self._pronto = threading.Event()
        self._fim = False
        self._hwnd = None
        self._hicon = None
        self._nid = None
        self._icone_visivel = False
        self._wndproc = None
        self._classe = None
        self._nome_classe = ""
        self._thread: threading.Thread | None = None
        if not disponivel():
            return
        self._thread = threading.Thread(target=self._loop, name="bandeja", daemon=True)
        self._thread.start()

    def mostrar(self, dica: str = "Simulador CAN") -> bool:
        if not disponivel():
            return False
        if not self._pronto.wait(2.0) or not self._hwnd:
            return False
        caixa: queue.Queue = queue.Queue(maxsize=1)
        self._fila.put(("mostrar", caixa, dica or "Simulador CAN"))
        try:
            return bool(caixa.get(timeout=2.0))
        except queue.Empty:
            return False

    def remover(self) -> None:
        if not disponivel() or not self._hwnd:
            return
        caixa: queue.Queue = queue.Queue(maxsize=1)
        self._fila.put(("remover", caixa, ""))
        try:
            caixa.get(timeout=2.0)
        except queue.Empty:
            pass

    def encerrar(self) -> None:
        if not disponivel():
            return
        if self._thread is None:
            return
        if self._pronto.wait(0.5) and self._hwnd:
            caixa: queue.Queue = queue.Queue(maxsize=1)
            self._fila.put(("encerrar", caixa, ""))
            try:
                caixa.get(timeout=1.0)
            except queue.Empty:
                self._fim = True
        else:
            self._fim = True
        self._thread.join(timeout=2.0)

    def _loop(self) -> None:
        try:
            _garantir_api()
            self._criar_janela()
        finally:
            self._pronto.set()
        if not self._hwnd:
            return
        mensagem = wintypes.MSG()
        while not self._fim:
            self._drenar()
            if self._fim:
                break
            if user32.PeekMessageW(ctypes.byref(mensagem), None, 0, 0, PM_REMOVE):
                if mensagem.message == WM_QUIT:
                    break
                user32.TranslateMessage(ctypes.byref(mensagem))
                user32.DispatchMessageW(ctypes.byref(mensagem))
            else:
                time.sleep(0.05)
        self._remover_icone()
        if self._hwnd:
            user32.DestroyWindow(self._hwnd)
            self._hwnd = None

    def _criar_janela(self) -> None:
        self._wndproc = WNDPROC(self._proc)
        self._nome_classe = f"SimuladorCANBandeja{os.getpid()}"
        instancia = kernel32.GetModuleHandleW(None)
        classe = WNDCLASSW()
        classe.lpfnWndProc = self._wndproc
        classe.hInstance = instancia
        classe.lpszClassName = self._nome_classe
        self._classe = classe
        if not user32.RegisterClassW(ctypes.byref(classe)):
            return
        hwnd = user32.CreateWindowExW(
            0,
            self._nome_classe,
            "SimuladorCAN",
            0,
            0,
            0,
            0,
            0,
            None,
            None,
            instancia,
            None,
        )
        if not hwnd:
            return
        self._hwnd = hwnd
        self._hicon = user32.LoadIconW(None, ctypes.c_void_p(IDI_APPLICATION))
        nid = NOTIFYICONDATAW()
        nid.cbSize = ctypes.sizeof(NOTIFYICONDATAW)
        nid.hWnd = hwnd
        nid.uID = 1
        nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP
        nid.uCallbackMessage = WM_APP
        nid.hIcon = self._hicon
        nid.szTip = "Simulador CAN"
        self._nid = nid

    def _drenar(self) -> None:
        while True:
            try:
                comando, caixa, extra = self._fila.get_nowait()
            except queue.Empty:
                return
            try:
                if comando == "mostrar":
                    caixa.put(self._aplicar_icone(extra))
                elif comando == "remover":
                    self._remover_icone()
                    caixa.put(True)
                elif comando == "encerrar":
                    self._fim = True
                    caixa.put(True)
                else:
                    caixa.put(False)
            except Exception:
                try:
                    caixa.put(False)
                except Exception:
                    pass

    def _aplicar_icone(self, dica: str) -> bool:
        if self._nid is None:
            return False
        self._nid.szTip = (dica or "Simulador CAN")[:127]
        acao = NIM_MODIFY if self._icone_visivel else NIM_ADD
        ok = bool(shell32.Shell_NotifyIconW(acao, ctypes.byref(self._nid)))
        if ok:
            self._icone_visivel = True
        return ok

    def _remover_icone(self) -> None:
        if self._nid is not None and self._icone_visivel:
            shell32.Shell_NotifyIconW(NIM_DELETE, ctypes.byref(self._nid))
        self._icone_visivel = False

    def _proc(self, hwnd, msg, wparam, lparam):
        if msg == WM_APP:
            evento = int(lparam) & 0xFFFFFFFF
            if evento in (WM_LBUTTONUP, WM_LBUTTONDBLCLK):
                self._ao_restaurar()
            elif evento == WM_RBUTTONUP:
                self._menu(hwnd)
            return 0
        if msg == WM_DESTROY:
            user32.PostQuitMessage(0)
            return 0
        return user32.DefWindowProcW(hwnd, msg, wparam, lparam)

    def _menu(self, hwnd) -> None:
        menu = user32.CreatePopupMenu()
        if not menu:
            return
        user32.AppendMenuW(menu, MF_STRING, ID_ABRIR, "Abrir")
        user32.AppendMenuW(menu, MF_STRING, ID_SAIR, "Sair")
        user32.SetForegroundWindow(hwnd)
        ponto = wintypes.POINT()
        user32.GetCursorPos(ctypes.byref(ponto))
        comando = user32.TrackPopupMenu(
            menu,
            TPM_RIGHTALIGN | TPM_RETURNCMD | TPM_NONOTIFY,
            ponto.x,
            ponto.y,
            0,
            hwnd,
            None,
        )
        user32.PostMessageW(hwnd, WM_NULL, 0, 0)
        user32.DestroyMenu(menu)
        if comando == ID_ABRIR:
            self._ao_restaurar()
        elif comando == ID_SAIR:
            self._ao_sair()
