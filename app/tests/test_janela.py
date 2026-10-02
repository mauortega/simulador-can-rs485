"""Abre a janela de verdade contra uma porta serial falsa."""

from __future__ import annotations

import os
import sys
import tempfile
import time
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import nucleo
import simulador_can
from test_nucleo import PortaFalsa


def _tem_display() -> bool:
    try:
        import tkinter as tk

        root = tk.Tk()
        root.withdraw()
        root.update_idletasks()
        root.destroy()
        return True
    except Exception:
        return False


def _esperar(root, condicao, timeout=2.0) -> bool:
    fim = time.time() + timeout
    while time.time() < fim:
        try:
            root.update()
        except Exception:
            return False
        if condicao():
            return True
        time.sleep(0.02)
    return False


@unittest.skipUnless(_tem_display(), "sem display para o tkinter")
class TestJanela(unittest.TestCase):
    def setUp(self):
        import tkinter as tk

        self.tmp = tempfile.TemporaryDirectory()
        os.environ["XDG_CONFIG_HOME"] = self.tmp.name
        self.porta = PortaFalsa()
        self.root = tk.Tk()
        self.app = simulador_can.Aplicativo(
            self.root,
            nucleo.Preferencias(porta="LOOP", baud=115200, sempre_no_topo=False),
            fabrica_serial=lambda _porta, _baud: self.porta,
        )
        self.root.update()

    def tearDown(self):
        self.app.encerrar()
        try:
            self.root.destroy()
        except Exception:
            pass
        self.tmp.cleanup()
        os.environ.pop("XDG_CONFIG_HOME", None)

    def test_janela_compacta_e_fluxo_de_velocidade(self):
        self.root.update_idletasks()
        self.assertLessEqual(self.root.winfo_width(), 320)
        self.assertGreaterEqual(self.root.winfo_width(), 240)
        self.assertLessEqual(self.root.winfo_height(), 210)
        self.assertGreaterEqual(self.root.winfo_height(), 130)
        self.assertEqual(self.app.rotulo_status.cget("text"), "desconectado")
        self.assertEqual(str(self.app.botao_250.cget("state")), "disabled")

        self.app.combo_porta.set("LOOP")
        self.app.botao_conexao.invoke()
        self.assertTrue(self.app.link.conectado)
        self.assertEqual(self.app.botao_conexao.cget("text"), "Desconectar")
        self.assertEqual(str(self.app.botao_250.cget("state")), "normal")

        self.app.botao_500.invoke()
        self.assertTrue(bytes(self.porta.escrita).endswith(b"can500\r\n"))
        self.assertEqual(self.app.estado.status, "aguardando")
        self.assertEqual(self.app.botao_500.cget("style"), "VelWait.TButton")

        self.porta.injetar(b"CAN 500 kbit/s: OK | Mercedes\r\n")
        self.assertTrue(_esperar(self.root, lambda: self.app.estado.status == "500 · Mercedes"))
        self.assertEqual(self.app.botao_500.cget("style"), "VelOn.TButton")
        self.assertEqual(self.app.botao_250.cget("style"), "Vel.TButton")
        self.assertEqual(self.root.title(), "CAN 500")

        self.porta.injetar(b"[CAN] VW250 updates=2 TX_OK=1 TX_FAIL=0 REC=0 RPM=0 Vel=0\r\n")
        self.assertTrue(_esperar(self.root, lambda: self.app.estado.kbps_confirmado == 250))
        self.assertEqual(self.app.botao_250.cget("style"), "VelOn.TButton")
        self.assertEqual(self.app.rotulo_status.cget("text"), "250 · J1939")

        self.app.botao_conexao.invoke()
        self.assertFalse(self.app.link.conectado)
        self.assertEqual(self.app.rotulo_status.cget("text"), "desconectado")
        self.assertEqual(self.app.botao_250.cget("style"), "VelOn.TButton")

    def test_sem_porta_mostra_status(self):
        self.app.combo_porta.set("")
        self.app.botao_conexao.invoke()
        self.assertEqual(self.app.rotulo_status.cget("text"), "sem porta")
        self.assertFalse(self.app.link.conectado)


if __name__ == "__main__":
    unittest.main()
