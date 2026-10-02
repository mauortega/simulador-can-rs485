"""Testes do protocolo serial e do estado do painel, sem hardware."""

from __future__ import annotations

import os
import pty
import queue
import sys
import tempfile
import threading
import time
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import nucleo


class PortaFalsa:
    def __init__(self, timeout=0.05):
        self.escrita = bytearray()
        self.timeout = timeout
        self.is_open = True
        self._entrada = bytearray()
        self._cond = threading.Condition()

    def write(self, dados):
        if not self.is_open:
            raise RuntimeError("fechada")
        self.escrita.extend(dados)
        return len(dados)

    def flush(self):
        return None

    def read(self, n):
        with self._cond:
            if not self._entrada and self.is_open:
                self._cond.wait(timeout=self.timeout or 0.05)
            if not self._entrada:
                return b""
            trecho = bytes(self._entrada[:n])
            del self._entrada[:n]
            return trecho

    def injetar(self, dados: bytes) -> None:
        with self._cond:
            self._entrada.extend(dados)
            self._cond.notify_all()

    def close(self):
        self.is_open = False
        with self._cond:
            self._cond.notify_all()


class TestProtocolo(unittest.TestCase):
    def test_comando_canonico(self):
        self.assertEqual(nucleo.comando_bitrate(250), b"can250\r\n")
        self.assertEqual(nucleo.comando_bitrate(500), b"can500\r\n")
        with self.assertRaises(ValueError):
            nucleo.comando_bitrate(125)

    def test_confirmacao_do_firmware(self):
        ok250 = nucleo.interpretar_linha("CAN 250 kbit/s: OK | J1939")
        ok500 = nucleo.interpretar_linha("CAN 500 kbit/s: OK | Mercedes")
        self.assertEqual(ok250, nucleo.Evento("ok", 250, "J1939", "CAN 250 kbit/s: OK | J1939"))
        self.assertEqual(ok500, nucleo.Evento("ok", 500, "Mercedes", "CAN 500 kbit/s: OK | Mercedes"))

    def test_erro_e_comando_invalido(self):
        erro = nucleo.interpretar_linha("CAN 250 kbit/s: ERRO")
        self.assertEqual(erro.tipo, "erro")
        self.assertEqual(erro.kbps, 250)
        invalido = nucleo.interpretar_linha("Comando invalido. Use can250 ou can500.")
        self.assertEqual(invalido.tipo, "invalido")

    def test_estatistica_e_boot_sem_confundir_o_banner(self):
        vw = nucleo.interpretar_linha("[CAN] VW250 updates=3 TX_OK=10 TX_FAIL=0 REC=0 RPM=800 Vel=0")
        mb = nucleo.interpretar_linha("[CAN] MB500 updates=4 TX_OK=1 TX_FAIL=0 REC=1 RPM=900 Vel=12")
        boot = nucleo.interpretar_linha("[CAN] begin=OK code=0 | CS=8 | bitrate=250000 | clock=8000000")
        self.assertEqual((vw.tipo, vw.kbps, vw.perfil), ("modo", 250, "J1939"))
        self.assertEqual((mb.tipo, mb.kbps, mb.perfil), ("modo", 500, "Mercedes"))
        self.assertEqual((boot.tipo, boot.kbps), ("modo", 250))
        self.assertIsNone(nucleo.interpretar_linha("=== CAN: can250 = J1939 | can500 = Mercedes ==="))
        self.assertIsNone(nucleo.interpretar_linha("Comandos CAN: can250 ou can500 + Enter"))
        self.assertIsNone(nucleo.interpretar_linha("CAN enabled buses init OK"))

    def test_baud_padrao_e_o_do_firmware(self):
        self.assertEqual(nucleo.BAUD_PADRAO, 115200)


class TestEstado(unittest.TestCase):
    def test_confirmacao_destaca_o_botao(self):
        estado = nucleo.EstadoPainel()
        estado.ao_conectar()
        estado.ao_solicitar(500)
        self.assertEqual(estado.botao_destaque(500), "espera")
        self.assertEqual(estado.status, "aguardando")
        estado.ao_evento(nucleo.Evento("ok", 500, "Mercedes"))
        self.assertEqual(estado.kbps_confirmado, 500)
        self.assertEqual(estado.botao_destaque(500), "ativo")
        self.assertEqual(estado.botao_destaque(250), "normal")
        self.assertEqual(estado.status, "500 · Mercedes")

    def test_estatistica_antiga_nao_apaga_pedido(self):
        estado = nucleo.EstadoPainel()
        estado.ao_conectar()
        estado.ao_evento(nucleo.Evento("modo", 250, "J1939"))
        estado.ao_solicitar(500)
        estado.ao_evento(nucleo.Evento("modo", 250, "J1939"))
        self.assertEqual(estado.kbps_pendente, 500)
        self.assertEqual(estado.kbps_confirmado, 250)
        estado.ao_evento(nucleo.Evento("erro", 500, None))
        self.assertIsNone(estado.kbps_pendente)
        self.assertEqual(estado.kbps_confirmado, 250)
        self.assertEqual(estado.status, "erro")
        self.assertEqual(estado.botao_destaque(250), "ativo")

    def test_evento_com_cabo_solto_e_ignorado(self):
        estado = nucleo.EstadoPainel()
        estado.ao_evento(nucleo.Evento("ok", 500, "Mercedes"))
        self.assertIsNone(estado.kbps_confirmado)


class TestLink(unittest.TestCase):
    def test_envia_can250_e_junta_linha_partida(self):
        porta = PortaFalsa()
        recebidos = queue.Queue()
        link = nucleo.LinkSerial(recebidos.put, recebidos.put, fabrica=lambda _p, _b: porta)
        link.conectar("COM11", 115200)
        self.addCleanup(link.desconectar)
        link.enviar_kbps(250)
        self.assertTrue(bytes(porta.escrita).endswith(b"can250\r\n"))
        porta.injetar(b"CAN 250 kbit/")
        porta.injetar(b"s: OK | J1939\r\n[CAN] MB500 updates=1\r\n")
        primeiro = recebidos.get(timeout=2)
        segundo = recebidos.get(timeout=2)
        self.assertEqual(primeiro.kbps, 250)
        self.assertEqual(primeiro.tipo, "ok")
        self.assertEqual(segundo.kbps, 500)
        self.assertEqual(segundo.perfil, "Mercedes")

    def test_enviar_sem_conexao_falha(self):
        link = nucleo.LinkSerial(lambda _e: None, lambda _m: None, fabrica=lambda _p, _b: PortaFalsa())
        with self.assertRaises(nucleo.LinkErro):
            link.enviar_kbps(250)

    @unittest.skipUnless(os.name == "posix", "pty so existe em posix")
    def test_pty_fala_o_protocolo_real(self):
        master, slave = pty.openpty()
        self.addCleanup(os.close, master)
        self.addCleanup(os.close, slave)
        nome = os.ttyname(slave)
        recebidos = queue.Queue()
        link = nucleo.LinkSerial(recebidos.put, lambda msg: recebidos.put(msg))
        link.conectar(nome, nucleo.BAUD_PADRAO)
        self.addCleanup(link.desconectar)
        link.enviar_kbps(500)
        bruto = _ler_ate(master, b"can500", 2)
        self.assertIn(b"can500\r\n", bruto)
        os.write(master, b"CAN 500 kbit/s: OK | Mercedes\r\n")
        evento = recebidos.get(timeout=2)
        self.assertEqual(evento, nucleo.Evento("ok", 500, "Mercedes", "CAN 500 kbit/s: OK | Mercedes"))


class TestPreferencias(unittest.TestCase):
    def test_roundtrip_e_json_ruim(self):
        with tempfile.TemporaryDirectory() as tmp:
            os.environ["XDG_CONFIG_HOME"] = tmp
            self.addCleanup(os.environ.pop, "XDG_CONFIG_HOME", None)
            prefs = nucleo.Preferencias(porta="COM11", baud=115200, sempre_no_topo=False, geometria="260x150+10+20")
            nucleo.salvar(prefs)
            self.assertEqual(nucleo.carregar(), prefs)
            caminho = nucleo.caminho_config()
            caminho.write_text("{", encoding="utf-8")
            self.assertEqual(nucleo.carregar(), nucleo.Preferencias())

    def test_lista_mantem_porta_salva(self):
        self.assertEqual(nucleo.montar_lista_portas(["COM1", "COM10", "COM2"], "COM11"), ["COM11", "COM1", "COM10", "COM2"])

    def test_geometria(self):
        self.assertTrue(nucleo.geometria_valida("260x150+10+20"))
        self.assertTrue(nucleo.geometria_valida("260x150+-8+20"))
        self.assertFalse(nucleo.geometria_valida("260x150"))
        self.assertFalse(nucleo.geometria_valida(""))


def _ler_ate(fd: int, trecho: bytes, timeout: float) -> bytes:
    import select

    buf = bytearray()
    fim = time.time() + timeout
    while time.time() < fim and trecho not in buf:
        pronto, _, _ = select.select([fd], [], [], 0.1)
        if pronto:
            buf.extend(os.read(fd, 256))
    return bytes(buf)


if __name__ == "__main__":
    unittest.main()
