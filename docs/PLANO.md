# Emuland: plano do projeto

Um único app para PS5 desbloqueado que reúne os emuladores nativos publicados no GitHub. O usuário
instala o Emuland uma vez e, a partir dele, instala, atualiza e abre cada emulador, e organiza as
pastas de jogos, BIOS, cheats e mods de cada um.

## 1. O que os três primeiros emuladores têm em comum

| | PSXS5 | PS5SX2 | ProsperoEden |
|---|---|---|---|
| Sistema | PlayStation 1 | PlayStation 2 | Switch (Eden) |
| Repositório | SynoPiia/PSXS5 | Swordpdf/PS5SX2 | blackbearreloaded/ProsperoEden |
| Title ID | `PPSA97510` | `PPSA99203` | `PPSA99008` |
| App instalado em | `/data/homebrew/PPSA97510/` | `/data/homebrew/PPSA99203/` | `/data/homebrew/PPSA99008/` |
| Dados do usuário | `/data/PSXS5/` | `/data/PCSX2/` | `/data/prosperoeden/` + pasta de jogos escolhida |
| Jogos | `games/<jogo>/` | `games/` | `<pasta>/roms/` |
| BIOS / firmware | `bios/` (opcional) | `bios/` (obrigatória) | `keys/` + `firmware/` (obrigatórios) |
| Cheats / patches | `games/<jogo>/cheats.cht` | `cheats/`, `patches/*.pnach` | `mods/<title ID>/` |
| Atualização própria | Sim, pelo GitHub | Sim, por `PS5SX2Installer.elf` | Sim, pelo homebrew.page + `self-updater.elf` |
| Requisitos | etaHEN + kstuff, ShadowMountPlus | kstuff, `PS5SX2Helper.elf`, ShadowMountPlus | etaHEN + kstuff, ShadowMountPlus |

Conclusões que guiam a arquitetura:

1. **Todos seguem o mesmo padrão de instalação:** um ZIP de release com a pasta do title ID, copiada
   para `/data/homebrew/` com permissão 777, que o ShadowMountPlus põe na tela inicial. Instalar e
   atualizar qualquer um deles é a mesma operação.
2. **A versão instalada é legível:** `contentVersion` em `sce_sys/param.json` (ex.: `01.002.000`).
3. **Os dados ficam fora da pasta do app**, então atualizar não apaga jogos, saves nem BIOS.
4. **PSXS5 e ProsperoEden usam o [ps5-native-app-boilerplate](https://github.com/blackbearreloaded/ps5-native-app-boilerplate)**
   (C/C++, SDL2, curl). O Emuland usa a mesma base: o app de teste (`app/`) provou o caminho no
   console, e o app principal (`hub/`) usa o kit de interface do mesmo autor,
   [ps5-homebrew-ui](https://github.com/blackbearreloaded/ps5-homebrew-ui) (OpenGL), como a
   documentação do template recomenda para interfaces de verdade.
5. **Já existe código de atualização para estudar:** `PSXS5/src/update.c` e
   `PS5SX2/ps5/installer/src/github.c` (ambos GPL-3.0, compatível com o Emuland se ele também for GPL-3.0).

## 2. Arquitetura

```
┌──────────────────────── Emuland (app PS5, PPSA9xxxx) ────────────────────────┐
│  Interface (ps5-homebrew-ui: OpenGL 4.6, controle DualSense)                 │
│    Início · Biblioteca de emuladores · Pastas · Atualizações · Ajustes       │
│                                                                              │
│  Catálogo ─────► lê emuladores.json (GitHub do Emuland, cache local)         │
│  Instalador ───► GitHub Releases API → ZIP → SHA-256 → /data/homebrew/<TID>  │
│  Pastas ───────► cria, verifica e importa (USB → pastas de cada emulador)    │
│  Lançador ─────► abre o emulador pelo title ID                               │
└──────────────────────────────────────────────────────────────────────────────┘
```

### Catálogo (`catalog/emuladores.json`)

**Hoje (14/10/2026), embutido no app** ([`hub/src/emuland/model.cpp`](../hub/src/emuland/model.cpp),
espelhado no JSON): os emuladores nativos para PS5 que publicam release no GitHub.

| Fabricante | Emulador | Sistemas | Observação |
|---|---|---|---|
| Sony | PSXS5, SwanStationPS5 | PS1 | |
| Sony | PS5SX2 | PS2 | |
| Sony | RPCS3 (lavavex) | PS3 | só código-fonte: sem botão de instalar; reconhecido se instalado à mão (PPSA99300) |
| Nintendo | ProsperoEden | Switch | só pré-releases |
| Nintendo | PS5CEMU-HAR | Wii U, 3DS, DS | |
| Microsoft | XPSemu | Xbox original | alpha |
| Vários | RetroArch (Mihawk) | N64, GameCube/Wii, PSP, SNES, GBA, Mega Drive, arcade... (33 núcleos) | 790 MB; BIOS ficam em `system/` dentro do app, mescladas na atualização |

Ficaram de fora: VITA5 (não emula, transmite um Vita pelo USB) e uma "re-upload" do RPCS3 por
terceiros (não é release do autor).

**Depois, online:** o mesmo JSON publicado no repositório do Emuland no GitHub; o app usa o mais
novo entre o embutido e o online. Depende de o usuário criar o repositório.

Cada emulador é uma entrada de dados, sem código específico. Adicionar um novo emulador é um pull
request no catálogo, sem nova versão do app. Cada entrada diz: repositório, title ID, qual arquivo
do release baixar, onde ficam os dados, quais pastas existem e o que cada uma precisa conter, e os
requisitos (payloads). O rascunho com os três está em [`catalog/emuladores.json`](../catalog/emuladores.json).

### Instalar e atualizar

Implementado em [`hub/src/platform/ps5/installer/`](../hub/src/platform/ps5/installer). Teste no
PC com downloads reais: `wsl -- bash hub/tools/installer-test.sh` (instala os três numa pasta
temporária e confere troca, backup, permissões e recusa com o emulador aberto). O registro do que
o Emuland instalou fica em `/data/emuland/installed/<TID>.txt` (tag, arquivo, SHA-256); o último
release visto, em `/data/emuland/releases.txt`; a versão anterior, em `/data/emuland/backup/<TID>`.
As tags não seguem a `contentVersion` (PSXS5 v2.2.0 traz 01.000.000; PS5SX2 vk-285-171 traz
02.100.000; só o ProsperoEden coincide), por isso o registro.


1. Consulta `GET /repos/<repo>/releases/latest` (uma vez ao abrir e no botão "Procurar atualizações").
   O ProsperoEden só publica pré-releases (o `latest` dele responde 404), então o catálogo marca
   `incluir_prerelease` e o Emuland usa `GET /repos/<repo>/releases` e pega o primeiro.
2. Compara com o `contentVersion` instalado.
3. Baixa o ZIP para `/data/emuland/downloads/`, confere o SHA-256 (a API do GitHub já informa o
   `digest` de cada arquivo de release; conferido nos três em 13/10/2026) e descompacta numa pasta
   temporária.
4. Guarda a versão atual em `/data/emuland/backup/<TID>/` (permite voltar uma versão).
5. Substitui `/data/homebrew/<TID>/` e aplica permissão 777.

O emulador nunca está aberto durante a atualização (o Emuland é outro app), então não precisa de
auxiliar externo como o `self-updater.elf`. As atualizações próprias de cada emulador continuam
funcionando: os dois mecanismos escrevem no mesmo lugar, e o Emuland só lê a versão.

### Pastas

O Emuland **não move nem duplica** os dados dos emuladores: cada um continua lendo onde já lê. Ele:

- cria as pastas que faltam;
- mostra o que cada pasta tem (ex.: "BIOS PS2: nenhuma encontrada", "3 jogos", "prod.keys ok");
- **importa de um pendrive** com um layout único:
  `Emuland/ps1/{jogos,bios}`, `Emuland/ps2/{jogos,bios,cheats,patches}`,
  `Emuland/switch/{jogos,keys,firmware,mods}` → copia para o lugar de cada emulador;
- (depois) oferece uma página no celular, por QR code, para enviar arquivos pela rede, como PSXS5 e
  PS5SX2 já fazem com os ajustes.

### Abrir um emulador

Pelo title ID, com `sceSystemServiceLaunchApp` (confirmado no console no teste 3; ver
[TESTE-PS5.md](TESTE-PS5.md)). `sceLncUtilLaunchApp` fecha o app quando chamada de dentro dele.
Antes, o Emuland pede acesso a `/data` ao HEN (LegacyJB/etaHEN), como o PSXS5.

### Requisitos do console

Verificação ao abrir: ShadowMountPlus ativo, acesso a `/data`, e payloads exigidos por emulador
(ex.: `PS5SX2Helper.elf` no autoload). Mostra o que falta com instruções; não instala payloads
sozinho na primeira versão.

## 3. Regras

- **Nenhuma BIOS, chave, firmware ou jogo é baixado ou distribuído.** O Emuland só cria pastas e
  copia arquivos que o próprio usuário fornece. Mesma regra dos três projetos.
- **Os binários vêm dos releases oficiais de cada autor**, sem recompilar nem hospedar cópias.
  Cada emulador aparece com nome, autor, licença e link do repositório.
- Avisar os autores antes de publicar o Emuland.

## 4. Fases

Cada fase só começa com sua ordem.

| Fase | Entrega |
|---|---|
| **0. Plano** (esta) | Este documento, catálogo rascunho, `design.md` |
| **1. Visual** | Protótipo navegável em HTML, 1920×1080, controlado por teclado/controle, para aprovar telas |
| **2. Catálogo e atualizador no PC** | Script (Python) que lê o catálogo, consulta releases e prepara/instala por FTP, para validar a lógica sem compilar para o PS5 |
| **3. App nativo PS5** | Emuland sobre o boilerplate: biblioteca, instalar, atualizar, abrir. Feito: teste no console (`app/`), o visual de streaming (`hub/`) e instalar/atualizar pelo GitHub (testado no PC com downloads reais; falta o console) |
| **4. Pastas** | Verificação, criação e importação do pendrive |
| **5. Extras** | Página no celular, novos emuladores (PSP, GameCube/Wii, N64...), voltar versão |

## 5. Pontos em aberto

- Title ID próprio do Emuland (escolher um livre na faixa `PPSA9xxxx` usada por homebrew).
- PS5SX2 hoje recomenda o próprio instalador por payload; confirmar com o autor que instalar o ZIP
  do release direto é suportado.
- Estrutura interna dos ZIPs do PS5SX2 e do ProsperoEden (o campo `pasta_no_zip` só foi
  confirmado no PSXS5, pelo README dele); conferir na fase 2.
- Firmware de referência para testes (os projetos usam 11.40 e 13.60).
