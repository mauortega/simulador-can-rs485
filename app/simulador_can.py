"""Painel compacto para escolher 250 ou 500 kbps no simulador CAN.

Rode com `python simulador_can.py` ou `pythonw simulador_can.py` no Windows.
"""

from __future__ import annotations

import ctypes
import os
import queue
import tkinter as tk
from tkinter import ttk

import bandeja
import nucleo

LARGURA = 260
ALTURA = 150
COR_FUNDO = "#f2f2f2"
_STATUS_ERRO = {
    "erro",
    "sem resposta",
    "recusado",
    "falha na porta",
    "falha ao enviar",
    "porta em uso",
    "porta ausente",
    "sem porta",
    "baud inválido",
}
_STATUS_OK = {"250 · J1939", "500 · Mercedes"}


def _preparar_dpi() -> None:
    if os.name != "nt":
        return
    try:
        ctypes.windll.shcore.SetProcessDpiAwareness(1)
    except Exception:
        try:
            ctypes.windll.user32.SetProcessDPIAware()
        except Exception:
            pass


def _aplicar_estilo(root: tk.Tk) -> ttk.Style:
    estilo = ttk.Style(root)
    try:
        estilo.theme_use("clam")
    except tk.TclError:
        pass
    fonte = ("Segoe UI", 9)
    fonte_botao = ("Segoe UI", 10, "bold")
    root.configure(bg=COR_FUNDO)
    estilo.configure(".", background=COR_FUNDO)
    estilo.configure("TFrame", background=COR_FUNDO)
    estilo.configure("TLabel", background=COR_FUNDO, font=fonte)
    estilo.configure("TCheckbutton", background=COR_FUNDO, font=fonte)
    estilo.configure("TButton", font=fonte, padding=(6, 2))
    estilo.configure("TCombobox", padding=1)
    estilo.configure(
        "Vel.TButton",
        font=fonte_botao,
        padding=(4, 8),
        background="#e7e7e7",
        foreground="#222222",
        bordercolor="#c8c8c8",
        focuscolor="#1b7f3a",
    )
    estilo.map(
        "Vel.TButton",
        background=[("pressed", "#d0d0d0"), ("active", "#dedede"), ("disabled", "#ececec")],
        foreground=[("disabled", "#8a8a8a")],
    )
    estilo.configure(
        "VelOn.TButton",
        font=fonte_botao,
        padding=(4, 8),
        background="#1b7f3a",
        foreground="#ffffff",
        bordercolor="#0f4d22",
        focuscolor="#ffffff",
    )
    estilo.map(
        "VelOn.TButton",
        background=[("pressed", "#14612c"), ("active", "#229246"), ("disabled", "#1b7f3a")],
        foreground=[("disabled", "#ffffff")],
    )
    estilo.configure(
        "VelWait.TButton",
        font=fonte_botao,
        padding=(4, 8),
        background="#d97706",
        foreground="#ffffff",
        bordercolor="#b45309",
        focuscolor="#ffffff",
    )
    estilo.map(
        "VelWait.TButton",
        background=[("pressed", "#b45309"), ("active", "#f59e0b"), ("disabled", "#d97706")],
        foreground=[("disabled", "#ffffff")],
    )
    estilo.configure("Status.TLabel", font=("Segoe UI", 8), background=COR_FUNDO, foreground="#666666")
    estilo.configure("StatusOk.TLabel", font=("Segoe UI", 8), background=COR_FUNDO, foreground="#14612c")
    estilo.configure("StatusErro.TLabel", font=("Segoe UI", 8), background=COR_FUNDO, foreground="#a12626")
    return estilo


class Aplicativo:
    def __init__(self, root: tk.Tk, prefs: nucleo.Preferencias | None = None, fabrica_serial=None):
        self.root = root
        self.prefs = prefs if prefs is not None else nucleo.carregar()
        self.estado = nucleo.EstadoPainel()
        self.link = nucleo.LinkSerial(self._evento_da_serial, self._erro_da_serial, fabrica_serial)
        self._fila_gui: queue.Queue = queue.Queue()
        self.bandeja = bandeja.Bandeja(
            ao_restaurar=lambda: self._na_thread_tk(self.restaurar),
            ao_sair=lambda: self._na_thread_tk(self._fechar),
        )
        self._pronto = False
        self._encerrado = False
        self._na_bandeja = False
        self._token_espera = 0
        self._job_salvar = None
        self.var_topo = tk.BooleanVar(value=self.prefs.sempre_no_topo)

        _aplicar_estilo(root)
        root.title("CAN")
        root.resizable(False, False)
        root.minsize(LARGURA, ALTURA)
        self._icone = tk.PhotoImage(width=16, height=16)
        self._icone.put("#1b7f3a", to=(0, 0, 16, 16))
        root.iconphoto(True, self._icone)

        self._montar()
        root.protocol("WM_DELETE_WINDOW", self._fechar)
        root.bind("<Configure>", self._ao_configurar)
        root.bind("<Unmap>", self._ao_unmap)
        self._aplicar_geometria()
        self._aplicar_topo()
        self._sincronizar()
        self._pronto = True
        self._job_bombear = self.root.after(30, self._bombear)

    def _montar(self) -> None:
        self.root.grid_columnconfigure(0, weight=1)
        self.root.grid_columnconfigure(1, weight=1)
        self.root.grid_rowconfigure(2, weight=1)

        linha_porta = ttk.Frame(self.root)
        linha_porta.grid(row=0, column=0, columnspan=2, sticky="ew", padx=6, pady=(6, 2))
        linha_porta.columnconfigure(0, weight=1)
        self.combo_porta = ttk.Combobox(linha_porta, state="normal")
        self.combo_porta.grid(row=0, column=0, sticky="ew")
        self.botao_atualizar = ttk.Button(linha_porta, text="⟳", width=3, command=self._atualizar_portas)
        self.botao_atualizar.grid(row=0, column=1, padx=(4, 0))

        linha_baud = ttk.Frame(self.root)
        linha_baud.grid(row=1, column=0, columnspan=2, sticky="ew", padx=6, pady=2)
        linha_baud.columnconfigure(1, weight=1)
        self.combo_baud = ttk.Combobox(linha_baud, width=8, state="readonly")
        self.combo_baud.grid(row=0, column=0, sticky="w")
        self.botao_conexao = ttk.Button(linha_baud, text="Conectar", width=12, command=self._alternar_conexao)
        self.botao_conexao.grid(row=0, column=1, sticky="e", padx=(4, 0))

        self.botao_250 = ttk.Button(self.root, text="250 kbps", style="Vel.TButton", command=lambda: self._clicar_velocidade(250))
        self.botao_500 = ttk.Button(self.root, text="500 kbps", style="Vel.TButton", command=lambda: self._clicar_velocidade(500))
        self.botao_250.grid(row=2, column=0, sticky="nsew", padx=(6, 3), pady=4)
        self.botao_500.grid(row=2, column=1, sticky="nsew", padx=(3, 6), pady=4)

        rodape = ttk.Frame(self.root)
        rodape.grid(row=3, column=0, columnspan=2, sticky="ew", padx=6, pady=(0, 4))
        rodape.columnconfigure(1, weight=1)
        self.check_topo = ttk.Checkbutton(rodape, text="No topo", variable=self.var_topo, command=self._ao_topo)
        self.check_topo.grid(row=0, column=0, sticky="w")
        self.rotulo_status = ttk.Label(rodape, text="desconectado", style="Status.TLabel", anchor="e")
        self.rotulo_status.grid(row=0, column=1, sticky="e")

        self._preencher_baud()
        self._atualizar_portas()

    def _preencher_baud(self) -> None:
        valores = [str(item) for item in nucleo.BAUDS]
        if str(self.prefs.baud) not in valores:
            valores.append(str(self.prefs.baud))
        self.combo_baud["values"] = valores
        self.combo_baud.set(str(self.prefs.baud))

    def _atualizar_portas(self) -> None:
        atual = self.combo_porta.get().strip()
        preferida = atual or self.prefs.porta
        portas = nucleo.montar_lista_portas(nucleo.listar_portas(), preferida)
        self.combo_porta["values"] = portas
        if atual:
            self.combo_porta.set(atual)
        elif self.prefs.porta:
            self.combo_porta.set(self.prefs.porta)
        elif len(portas) == 1:
            self.combo_porta.set(portas[0])

    def _aplicar_geometria(self) -> None:
        if nucleo.geometria_valida(self.prefs.geometria):
            self.root.geometry(self.prefs.geometria)
            return
        self.root.update_idletasks()
        x = max(0, self.root.winfo_screenwidth() - LARGURA - 16)
        y = 16
        self.root.geometry(f"{LARGURA}x{ALTURA}+{x}+{y}")

    def _aplicar_topo(self) -> None:
        try:
            self.root.attributes("-topmost", bool(self.var_topo.get()))
        except tk.TclError:
            pass

    def _ao_topo(self) -> None:
        self.prefs.sempre_no_topo = bool(self.var_topo.get())
        self._aplicar_topo()
        self._salvar_prefs()

    def _ao_configurar(self, event) -> None:
        if event.widget is not self.root or not self._pronto or self._na_bandeja:
            return
        if str(self.root.state()) != "normal":
            return
        geo = self.root.geometry()
        if not nucleo.geometria_valida(geo):
            return
        largura = int(geo.split("x", 1)[0])
        if largura < 200:
            return
        self.prefs.geometria = geo
        self._agendar_salvar()

    def _agendar_salvar(self) -> None:
        if self._job_salvar is not None:
            try:
                self.root.after_cancel(self._job_salvar)
            except tk.TclError:
                pass
        self._job_salvar = self.root.after(400, self._salvar_prefs)

    def _salvar_prefs(self) -> None:
        self._job_salvar = None
        if not self._pronto:
            return
        try:
            nucleo.salvar(self.prefs)
        except OSError:
            pass

    def _alternar_conexao(self) -> None:
        if self.link.conectado:
            self.link.desconectar()
            self._token_espera += 1
            self.estado.ao_desconectar()
            self._sincronizar()
            return
        porta = self.combo_porta.get().strip()
        if not porta:
            self.estado.definir_status("sem porta")
            self._sincronizar()
            return
        try:
            baud = int(self.combo_baud.get())
        except (TypeError, ValueError):
            self.estado.definir_status("baud inválido")
            self._sincronizar()
            return
        try:
            self.link.conectar(porta, baud)
        except Exception as exc:
            self.estado.ao_desconectar(nucleo.classificar_erro_porta(exc))
            self._sincronizar()
            return
        self.prefs.porta = porta
        self.prefs.baud = baud
        self.estado.ao_conectar()
        self._salvar_prefs()
        self._sincronizar()

    def _clicar_velocidade(self, kbps: int) -> None:
        if not self.link.conectado:
            return
        try:
            self.link.enviar_kbps(kbps)
        except Exception:
            self.estado.definir_status("falha ao enviar")
            self._sincronizar()
            return
        self.estado.ao_solicitar(kbps)
        self._armar_espera()
        self._sincronizar()

    def _armar_espera(self) -> None:
        self._token_espera += 1
        token = self._token_espera
        self.root.after(2000, lambda: self._vencer_espera(token))

    def _vencer_espera(self, token: int) -> None:
        if token != self._token_espera or self.estado.kbps_pendente is None:
            return
        self.estado.kbps_pendente = None
        self.estado.status = "sem resposta"
        self._sincronizar()

    def _evento_da_serial(self, evento: nucleo.Evento) -> None:
        self._na_thread_tk(lambda e=evento: self._aplicar_evento(e))

    def _erro_da_serial(self, _mensagem: str) -> None:
        self._na_thread_tk(self._perder_conexao)

    def _na_thread_tk(self, func) -> None:
        # A serial e a bandeja rodam fora da thread do Tk. A fila e drenada
        # por _bombear, porque o Tk recusa after() vindo de outra thread.
        self._fila_gui.put(func)

    def _bombear(self) -> None:
        if self._encerrado:
            return
        try:
            while True:
                func = self._fila_gui.get_nowait()
                func()
        except queue.Empty:
            pass
        try:
            self._job_bombear = self.root.after(30, self._bombear)
        except tk.TclError:
            self._job_bombear = None

    def _aplicar_evento(self, evento: nucleo.Evento) -> None:
        self.estado.ao_evento(evento)
        self._sincronizar()

    def _perder_conexao(self) -> None:
        if self._encerrado:
            return
        self._token_espera += 1
        self.link.desconectar()
        self.estado.ao_desconectar()
        self._sincronizar()

    def _sincronizar(self) -> None:
        conectado = self.link.conectado
        self.botao_conexao.configure(text="Desconectar" if conectado else "Conectar")
        estado_botao = "normal" if conectado else "disabled"
        for kbps, botao in ((250, self.botao_250), (500, self.botao_500)):
            destaque = self.estado.botao_destaque(kbps)
            if destaque == "ativo":
                aparencia = "VelOn.TButton"
            elif destaque == "espera":
                aparencia = "VelWait.TButton"
            else:
                aparencia = "Vel.TButton"
            botao.configure(style=aparencia, state=estado_botao)
        self.rotulo_status.configure(text=self.estado.status, style=self._estilo_status())
        if self.estado.kbps_confirmado == 250:
            self.root.title("CAN 250")
        elif self.estado.kbps_confirmado == 500:
            self.root.title("CAN 500")
        else:
            self.root.title("CAN")

    def _estilo_status(self) -> str:
        if self.estado.status in _STATUS_ERRO:
            return "StatusErro.TLabel"
        if self.estado.status in _STATUS_OK:
            return "StatusOk.TLabel"
        return "Status.TLabel"

    def _ao_unmap(self, event) -> None:
        if event.widget is not self.root:
            return
        if not bandeja.deve_ocultar_na_bandeja(bandeja.disponivel(), str(self.root.state()), self._na_bandeja):
            return
        dica = f"Simulador CAN — {self.estado.status}"
        if not self.bandeja.mostrar(dica):
            return
        self._na_bandeja = True
        self.root.withdraw()

    def restaurar(self) -> None:
        self._na_bandeja = False
        self.bandeja.remover()
        try:
            self.root.deiconify()
            self.root.state("normal")
            self.root.lift()
            self._aplicar_topo()
        except tk.TclError:
            pass

    def _cancelar_timers(self) -> None:
        self._job_bombear = None
        self._job_salvar = None
        try:
            identificadores = self.root.tk.call("after", "info")
        except tk.TclError:
            return
        for ident in identificadores:
            try:
                self.root.after_cancel(ident)
            except tk.TclError:
                pass

    def encerrar(self) -> None:
        if self._encerrado:
            return
        self._encerrado = True
        self._token_espera += 1
        self._cancelar_timers()
        try:
            self._salvar_prefs()
        except Exception:
            pass
        try:
            self.link.desconectar()
        except Exception:
            pass
        try:
            self.bandeja.encerrar()
        except Exception:
            pass

    def _fechar(self) -> None:
        self.encerrar()
        try:
            self.root.destroy()
        except tk.TclError:
            pass


def main() -> None:
    _preparar_dpi()
    root = tk.Tk()
    app = Aplicativo(root)
    try:
        root.mainloop()
    finally:
        app.encerrar()


if __name__ == "__main__":
    main()
