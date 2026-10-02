# Painel do simulador CAN

Janela pequena para Windows que escolhe a velocidade do barramento CAN no
simulador ATmega (250 ou 500 kbps) pela porta serial.

O firmware desta branch já entende os comandos. Esta pasta é a única inclusão:
nenhum arquivo fora de `app/` é alterado, e o aplicativo não grava a placa.

Se o texto da serial mudar no futuro, o único lugar a editar é `nucleo.py`
(`COMANDOS`, `BAUD_PADRAO` e as expressões logo abaixo). O restante do
programa só usa esses eventos.

## Requisitos

- Windows 10 ou mais recente.
- Python 3.11 ou mais recente. O uso previsto é o Python 3.13.
  Na instalação, marque **Add python.exe to PATH** e **tcl/tk and IDLE**.
- Firmware `simulador-atmega` já gravado no Arduino Nano.
- Cabo USB da placa (em geral um CH340).

Para só gerar o `.exe` e levar para outro computador, o destino não precisa
ter Python.

## Rodar

No PowerShell, a partir da pasta `app`:

```powershell
cd app
python -m pip install -r requirements.txt
pythonw simulador_can.py
```

Também dá para dar dois cliques em `rodar.bat`. `pythonw` abre o painel sem
janela de console. `python simulador_can.py` faz o mesmo e deixa o console
visível, o que ajuda se algo falhar ao abrir.

## Gerar um .exe único

Dois cliques em `gerar_exe.bat`, ou:

```powershell
cd app
.\gerar_exe.bat
```

O script instala `pyserial` e `PyInstaller` e gera:

```text
app\dist\SimuladorCAN.exe
```

Copie só esse arquivo. A primeira abertura demora alguns segundos porque o
PyInstaller descompacta o programa numa pasta temporária.

O comando equivalente, se quiser rodar à mão:

```powershell
python -m PyInstaller --noconfirm --clean --onefile --windowed --name SimuladorCAN simulador_can.py
```

`--windowed` tira o console. `--onefile` junta Python, tkinter e pyserial num
único executável.

## Uso

A janela nasce com cerca de 260×150 pixels, sem barra de menus, no canto
superior direito.

- A lista mostra as portas COM. O botão **⟳** atualiza a lista. Se a porta não
  aparecer, digite o nome (por exemplo `COM11`).
- A velocidade da serial começa em **115200**, que é o `Serial.begin(115200)`
  do firmware. As outras opções são 9600, 19200, 38400 e 57600.
- **Conectar** abre a porta. **Desconectar** fecha.
- **250 kbps** envia o perfil J1939/VW. **500 kbps** envia o perfil Mercedes.
  O botão da velocidade confirmada pelo firmware fica verde. Enquanto a
  resposta não chega, o botão clicado fica laranja.
- **No topo** mantém a janela acima dos outros programas.
- No Windows, minimizar manda o painel para a bandeja do sistema (ao lado do
  relógio). Um clique no ícone abre de novo. O botão direito oferece Abrir e
  Sair.

A última porta, o baud, a opção "no topo" e a posição da janela ficam em
`%APPDATA%\SimuladorCAN\config.json`.

O texto pequeno à direita mostra o estado, sem ocupar a janela:

| Texto | Significado |
|---|---|
| desconectado | porta fechada |
| conectado | porta aberta, velocidade ainda não confirmada |
| aguardando | comando enviado |
| 250 · J1939 | firmware confirmou 250 kbit/s |
| 500 · Mercedes | firmware confirmou 500 kbit/s |
| sem resposta | nada voltou em 2 segundos |
| erro | o MCP2515 não aceitou a troca |
| porta em uso | outro programa está com a COM |
| porta ausente | a COM não existe |
| sem porta | nenhuma porta foi escolhida |

Não abra o monitor serial do PlatformIO ao mesmo tempo. A COM só aceita um
programa.

## Por que Python + tkinter

É o caminho mais leve para quem já tem o Python 3.13: o tkinter vem no
instalador oficial e a única biblioteca extra para rodar é o pyserial. Um
programa em C# ou C++ poderia gerar um arquivo menor, mas pediria outro
compilador. A bandeja usa a API do Windows (`Shell_NotifyIcon`) direto, sem
pystray nem Pillow, para o `.exe` não carregar essas bibliotecas.

## Protocolo serial

Implementado em `src/CanApplication.cpp`. O aplicativo envia exatamente o que
esse código já compara.

- USB do Nano, 115200 baud, 8 bits de dados, sem paridade, 1 stop bit.
- Uma linha por comando, terminada por Enter (`\r`, `\n` ou `\r\n`).
- Maiúsculas e minúsculas valem igual. Espaço e tab são ignorados.
- Qualquer outro caractere zera o comando. O buffer do firmware tem 6 letras,
  o tamanho exato de `can250` e `can500`.

| Botão | Bytes | Efeito na placa |
|---|---|---|
| 250 kbps | `can250` + CR LF | MCP2515 a 250 kbit/s, simulação J1939/VW, LED A1 |
| 500 kbps | `can500` + CR LF | MCP2515 a 500 kbit/s, simulação Mercedes, LED A2 |

O `setBitrate` do MCP2515 aborta a transmissão em curso e reinicia o
controlador na velocidade nova, em modo normal e one-shot. A simulação passa
a enviar o perfil correspondente no mesmo `loop`.

Respostas, uma linha cada:

| Situação | Texto |
|---|---|
| 250 aceito | `CAN 250 kbit/s: OK \| J1939` |
| 500 aceito | `CAN 500 kbit/s: OK \| Mercedes` |
| falha ao reconfigurar | `CAN 250 kbit/s: ERRO` ou `CAN 500 kbit/s: ERRO` |
| texto desconhecido | `Comando invalido. Use can250 ou can500.` |

A cada 2 segundos o firmware também imprime o modo atual. O painel usa essa
linha para destacar o botão certo mesmo que ninguém tenha clicado ainda:

```text
[CAN] VW250 updates=3 TX_OK=10 TX_FAIL=0 REC=0 RPM=800 Vel=0
[CAN] MB500 updates=4 TX_OK=11 TX_FAIL=0 REC=0 RPM=900 Vel=12
```

Na inicialização, a linha de boot traz `bitrate=250000` ou `bitrate=500000`.
Ligado, o simulador começa em 250 kbit/s.

Exemplo:

```text
PC   -> Nano:  can500\r\n
Nano -> PC:    CAN 500 kbit/s: OK | Mercedes\r\n
```

O painel segura DTR e RTS em nível baixo ao abrir a porta, para não resetar o
Nano quando o driver obedece. Alguns CH340 pulsam o DTR mesmo assim e a placa
reinicia em 250 kbit/s. O painel acompanha a mensagem de boot ou a próxima
linha `[CAN]`.

## Testes

Na pasta `app`, com o pyserial instalado e uma tela disponível para a parte
da janela:

```powershell
python -m unittest discover -s tests -v
```

Os testes conferem as frases reais do firmware, o envio de `can250`/`can500`
numa porta falsa e numa porta serial virtual, e o destaque dos botões na
janela.
