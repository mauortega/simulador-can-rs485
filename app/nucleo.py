"""Protocolo serial do simulador e estado do painel.

O firmware (CanApplication) ja aceita, a 115200 baud, as linhas `can250` e
`can500`. Este modulo nao inventa comando novo: ele envia esses textos e
interpreta as respostas que o ATmega ja imprime.
"""

from __future__ import annotations

import json
import os
import re
import threading
from dataclasses import dataclass
from pathlib import Path

import serial
from serial.tools import list_ports

# Ponto unico de ajuste do protocolo. O restante do app nao monta essas frases.
BAUD_PADRAO = 115200
BAUDS = (9600, 19200, 38400, 57600, 115200)
COMANDOS = {
    250: b"can250\r\n",
    500: b"can500\r\n",
}
_PERFIL = {250: "J1939", 500: "Mercedes"}

_RE_ACK = re.compile(
    r"^CAN\s+(250|500)\s+kbit/s:\s+(OK|ERRO)"
    r"(?:\s+\|\s*(J1939|Mercedes))?\s*$",
    re.IGNORECASE,
)
_RE_MODO = re.compile(r"^\[CAN\]\s+(VW250|MB500)\b")
_RE_BOOT = re.compile(r"\bbitrate=(250000|500000)\b")
_RE_INVALIDO = re.compile(r"comando invalido", re.IGNORECASE)
_RE_GEOM = re.compile(r"^\d+x\d+[+-]-?\d+[+-]-?\d+$")
_RE_NUMERO_FINAL = re.compile(r"(\d+)$")


@dataclass(frozen=True)
class Evento:
    tipo: str
    kbps: int | None = None
    perfil: str | None = None
    bruto: str = ""


@dataclass
class Preferencias:
    porta: str = ""
    baud: int = BAUD_PADRAO
    sempre_no_topo: bool = True
    geometria: str = ""


class LinkErro(Exception):
    pass


def comando_bitrate(kbps: int) -> bytes:
    try:
        return COMANDOS[kbps]
    except KeyError as exc:
        raise ValueError(kbps) from exc


def interpretar_linha(linha: str) -> Evento | None:
    """Converte uma linha do firmware em evento. Linha desconhecida vira None."""
    texto = linha.strip()
    if not texto:
        return None

    ack = _RE_ACK.match(texto)
    if ack:
        kbps = int(ack.group(1))
        if ack.group(2).upper() == "OK":
            perfil = ack.group(3) or _PERFIL[kbps]
            return Evento("ok", kbps, perfil, texto)
        return Evento("erro", kbps, None, texto)

    modo = _RE_MODO.match(texto)
    if modo:
        if modo.group(1) == "MB500":
            return Evento("modo", 500, "Mercedes", texto)
        return Evento("modo", 250, "J1939", texto)

    boot = _RE_BOOT.search(texto)
    if boot:
        kbps = 250 if boot.group(1) == "250000" else 500
        return Evento("modo", kbps, _PERFIL[kbps], texto)

    if _RE_INVALIDO.search(texto):
        return Evento("invalido", None, None, texto)
    return None


def geometria_valida(valor: str) -> bool:
    return bool(_RE_GEOM.match(valor or ""))


def classificar_erro_porta(exc: BaseException) -> str:
    texto = str(exc).lower()
    if any(p in texto for p in ("access is denied", "permission", "busy", "em uso")):
        return "porta em uso"
    if any(p in texto for p in ("filenotfound", "cannot find", "no such", "not found", "file not found")):
        return "porta ausente"
    return "falha na porta"


def _chave_porta(nome: str) -> tuple[str, int]:
    encontrado = _RE_NUMERO_FINAL.search(nome)
    if not encontrado:
        return (nome.lower(), -1)
    return (nome[: encontrado.start()].lower(), int(encontrado.group(1)))


def listar_portas() -> list[str]:
    try:
        nomes = [porta.device for porta in list_ports.comports() if porta.device]
    except Exception:
        return []
    unicos = list(dict.fromkeys(nomes))
    return sorted(unicos, key=_chave_porta)


def montar_lista_portas(detectadas: list[str], preferida: str = "") -> list[str]:
    vistas: list[str] = []
    for nome in detectadas:
        if nome and nome not in vistas:
            vistas.append(nome)
    preferida = (preferida or "").strip()
    if preferida and preferida not in vistas:
        vistas.insert(0, preferida)
    return vistas


def caminho_config() -> Path:
    override = os.environ.get("SIMULADOR_CAN_CONFIG", "").strip()
    if override:
        return Path(override)
    if os.name == "nt":
        base = os.environ.get("APPDATA") or str(Path.home())
        return Path(base) / "SimuladorCAN" / "config.json"
    xdg = os.environ.get("XDG_CONFIG_HOME") or str(Path.home() / ".config")
    return Path(xdg) / "simulador-can" / "config.json"


def carregar() -> Preferencias:
    caminho = caminho_config()
    try:
        dados = json.loads(caminho.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError, UnicodeError):
        return Preferencias()
    if not isinstance(dados, dict):
        return Preferencias()

    porta = dados.get("porta", "")
    if not isinstance(porta, str):
        porta = ""
    porta = porta.strip()[:256]

    baud = dados.get("baud", BAUD_PADRAO)
    if isinstance(baud, bool) or not isinstance(baud, int) or not 1 <= baud <= 4_000_000:
        baud = BAUD_PADRAO

    topo = dados.get("sempre_no_topo", True)
    if not isinstance(topo, bool):
        topo = True

    geometria = dados.get("geometria", "")
    if not isinstance(geometria, str) or not geometria_valida(geometria):
        geometria = ""

    return Preferencias(porta=porta, baud=baud, sempre_no_topo=topo, geometria=geometria)


def salvar(prefs: Preferencias) -> None:
    caminho = caminho_config()
    caminho.parent.mkdir(parents=True, exist_ok=True)
    conteudo = {
        "porta": prefs.porta,
        "baud": int(prefs.baud),
        "sempre_no_topo": bool(prefs.sempre_no_topo),
        "geometria": prefs.geometria if geometria_valida(prefs.geometria) else "",
    }
    caminho.write_text(json.dumps(conteudo, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")


def abrir_porta(porta: str, baud: int):
    """Abre a serial com DTR e RTS baixos para nao resetar o Nano quando o driver deixa."""
    conexao = serial.Serial()
    conexao.port = porta
    conexao.baudrate = int(baud)
    conexao.timeout = 0.2
    conexao.write_timeout = 1.0
    conexao.exclusive = True
    _baixar_dtr_rts(conexao)
    conexao.open()
    _baixar_dtr_rts(conexao)
    return conexao


def _baixar_dtr_rts(conexao) -> None:
    try:
        conexao.dtr = False
        conexao.rts = False
    except OSError:
        pass


class LinkSerial:
    """Le a serial em outra thread e entrega eventos do firmware."""

    def __init__(self, ao_evento, ao_erro, fabrica=None):
        self._ao_evento = ao_evento
        self._ao_erro = ao_erro
        self._fabrica = fabrica or abrir_porta
        self._trava = threading.Lock()
        self._rodando = False
        self._ser = None
        self._thread: threading.Thread | None = None

    @property
    def conectado(self) -> bool:
        with self._trava:
            return self._rodando and self._ser is not None

    def conectar(self, porta: str, baud: int) -> None:
        self.desconectar()
        conexao = self._fabrica(porta, int(baud))
        thread = threading.Thread(target=self._ler, name="serial-can", daemon=True)
        with self._trava:
            self._ser = conexao
            self._rodando = True
            self._thread = thread
        thread.start()

    def desconectar(self) -> None:
        with self._trava:
            self._rodando = False
            conexao = self._ser
            self._ser = None
            thread = self._thread
            self._thread = None
        if conexao is not None:
            try:
                conexao.close()
            except Exception:
                pass
        if thread is not None and thread is not threading.current_thread():
            thread.join(timeout=1.5)

    def enviar_kbps(self, kbps: int) -> None:
        pacote = comando_bitrate(kbps)
        with self._trava:
            conexao = self._ser
            ativo = self._rodando
        if not ativo or conexao is None:
            raise LinkErro("desconectado")
        conexao.write(pacote)
        try:
            conexao.flush()
        except Exception:
            pass

    def _ler(self) -> None:
        buf = bytearray()
        while True:
            with self._trava:
                if not self._rodando:
                    return
                conexao = self._ser
            if conexao is None:
                return
            try:
                dados = conexao.read(128)
            except Exception as exc:
                with self._trava:
                    avisar = self._rodando
                    self._rodando = False
                    self._ser = None
                if avisar:
                    try:
                        self._ao_erro(str(exc))
                    except Exception:
                        pass
                return
            if dados:
                buf.extend(dados)
            while True:
                quebra = buf.find(b"\n")
                if quebra < 0:
                    break
                bruto = bytes(buf[:quebra])
                del buf[: quebra + 1]
                texto = bruto.decode("ascii", errors="replace")
                evento = interpretar_linha(texto)
                if evento is not None:
                    try:
                        self._ao_evento(evento)
                    except Exception:
                        pass
            if len(buf) > 2048:
                del buf[:-256]


class EstadoPainel:
    def __init__(self) -> None:
        self.conectado = False
        self.kbps_confirmado: int | None = None
        self.kbps_pendente: int | None = None
        self.perfil: str | None = None
        self.status = "desconectado"

    def ao_conectar(self) -> None:
        self.conectado = True
        self.kbps_pendente = None
        self.status = self._rotulo() if self.kbps_confirmado else "conectado"

    def ao_desconectar(self, status: str = "desconectado") -> None:
        self.conectado = False
        self.kbps_pendente = None
        self.status = status

    def definir_status(self, status: str) -> None:
        self.status = status

    def ao_solicitar(self, kbps: int) -> None:
        self.kbps_pendente = kbps
        self.status = "aguardando"

    def ao_evento(self, evento: Evento) -> None:
        if not self.conectado:
            return
        if evento.tipo == "ok":
            self._confirmar(evento.kbps, evento.perfil)
        elif evento.tipo == "erro":
            self.kbps_pendente = None
            self.status = "erro"
        elif evento.tipo == "modo":
            if self.kbps_pendente is not None and evento.kbps != self.kbps_pendente:
                return
            self._confirmar(evento.kbps, evento.perfil)
        elif evento.tipo == "invalido":
            self.kbps_pendente = None
            self.status = "recusado"

    def _confirmar(self, kbps: int | None, perfil: str | None) -> None:
        if kbps not in (250, 500):
            return
        self.kbps_confirmado = kbps
        self.perfil = perfil or _PERFIL[kbps]
        if self.kbps_pendente == kbps:
            self.kbps_pendente = None
        self.status = self._rotulo()

    def _rotulo(self) -> str:
        if self.kbps_confirmado == 500:
            return "500 · Mercedes"
        if self.kbps_confirmado == 250:
            return "250 · J1939"
        return "conectado"

    def botao_destaque(self, kbps: int) -> str:
        if self.kbps_pendente == kbps:
            return "espera"
        if self.kbps_pendente is None and self.kbps_confirmado == kbps:
            return "ativo"
        return "normal"
