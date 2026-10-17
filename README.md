<p align="center">
  <img src="docs/brand/manta-icon-512.png" width="128" alt="Ícone do Manta">
</p>

<h1 align="center">Manta</h1>

<p align="center">
  <b>Todos os emuladores do seu PS5 em um só lugar.</b><br>
  Instale, atualize, organize e jogue — sem FTP, sem PC, sem complicação.
</p>

<p align="center">
  <a href="../../releases/latest"><img alt="Release" src="https://img.shields.io/github/v/release/nagliostro/manta?label=vers%C3%A3o"></a>
  <img alt="Plataforma" src="https://img.shields.io/badge/plataforma-PS5%20homebrew-1E3A5F">
  <a href="LICENSE"><img alt="Licença" src="https://img.shields.io/badge/licen%C3%A7a-GPL--3.0-5B6878"></a>
</p>

<p align="center">
  <img src="docs/screenshots/inicio.png" width="900" alt="Tela inicial do Manta">
</p>

> **English:** Manta is a PS5 homebrew hub that installs, updates, removes and opens every native
> PS5 emulator from one place, with a game library and covers. The app speaks Portuguese and
> English (it follows the console's language). See [Installation](#instalação) — the steps are the
> same in any language.

---

## Por que o Manta existe

Cada emulador nativo de PS5 vive num repositório diferente, com seu próprio jeito de instalar,
suas próprias pastas e seu próprio ritmo de atualização. Para usar três emuladores era preciso
baixar três ZIPs no PC, copiar por FTP, acertar permissões, lembrar onde vai cada BIOS e voltar ao
GitHub de tempos em tempos para ver se saiu versão nova.

O Manta junta tudo isso em **um app só, no próprio console**:

- você instala o Manta uma vez;
- a partir dele, instala e atualiza cada emulador com um botão;
- copia jogos e BIOS de um pendrive sem saber o caminho de nada;
- e abre seus jogos de uma biblioteca única, com capas, como na tela do PS5.

## Funcionalidades

| | |
|---|---|
| **Catálogo de emuladores** | 8 emuladores nativos de PS5, organizados por fabricante, com ícone, arte, autor, licença e requisitos de cada um. |
| **Instalar e atualizar** | Baixa o release mais novo direto do GitHub, confere o **SHA-256** publicado, extrai e coloca no lugar. A versão anterior fica guardada até a nova estar pronta. "Atualizar todos" de uma vez. |
| **Desinstalar** | Remove só o app ("Manter meus arquivos") ou tudo, incluindo jogos, BIOS e saves ("Apagar tudo"). O ícone sai da tela inicial sozinho quando o Manta fecha. |
| **Biblioteca de jogos** | Todos os jogos de todos os emuladores instalados em uma grade de capas, com filtro por sistema. Discos de um mesmo jogo aparecem uma vez só. |
| **Capas automáticas** | Usa as capas que você já tem e, se quiser, baixa as que faltam do [libretro-thumbnails](https://github.com/libretro-thumbnails/libretro-thumbnails). |
| **Jogar direto** | Nos emuladores que aceitam (SwanStationPS5, PS5CEMU-HAR), ✕ abre o jogo direto, sem passar pelo menu do emulador. |
| **"Abrir em qual emulador?"** | Se mais de um emulador instalado roda o jogo (ex.: PSXS5 e SwanStationPS5 para PS1), o Manta pergunta em qual abrir e lembra da última escolha. |
| **Pastas** | Mostra cada pasta de cada emulador (jogos, BIOS, cheats, saves…), quantos arquivos tem e o que está faltando. Cria as que não existem. |
| **Importar do pendrive** | Copia de `Manta/<sistema>/<pasta>/` no pendrive ou HD externo para o lugar certo no console. Só copia: nada é apagado, e o que já está lá é pulado. |
| **Preparar pendrive** | Cria no pendrive a árvore de pastas dos emuladores instalados, com um `LEIA-ME.txt` dizendo o que vai em cada uma. |
| **Atualiza a si mesmo** | Uma versão nova do Manta aparece em Atualizações, como a dos emuladores. Ele baixa, confere e aplica a troca assim que você fecha o app. |
| **Catálogo online** | Emuladores novos chegam ao Manta sem precisar de versão nova do app: o catálogo vem do [`catalog/online.json`](catalog/online.json) deste repositório. |
| **Bilíngue** | Português e inglês, seguindo o idioma do console (ou escolha em Ajustes). |
| **Feito para a TV** | Interface no estilo de streaming, 60 fps, 4K, sons e movimento suaves, opção de reduzir movimento, contraste AA. |

### Emuladores do catálogo

| Emulador | Sistema | Autor | Instalação pelo Manta |
|---|---|---|---|
| [PSXS5](https://github.com/SynoPiia/PSXS5) | PlayStation | SynoPiia | ✅ |
| [SwanStationPS5](https://github.com/darkxex/SwanStationPS5) | PlayStation | AngelXex | ✅ (abre o jogo direto) |
| [PS5SX2](https://github.com/Swordpdf/PS5SX2) | PlayStation 2 | Swordpdf | ✅ |
| [RPCS3 PS5](https://github.com/lavavex/rpcs3-PS5) | PlayStation 3 | lavavex | ⚠️ só código-fonte; reconhecido se instalado à mão |
| [ProsperoEden](https://github.com/blackbearreloaded/ProsperoEden) | Nintendo Switch | blackbearreloaded | ✅ |
| [PS5CEMU-HAR](https://github.com/premohq/PS5CEMU-HAR) | Wii U · 3DS · DS | premohq | ✅ (abre o jogo direto) |
| [XPSemu](https://github.com/ZiZc3/XPSemu) | Xbox | AZiZ | ✅ |
| [RetroArch PS5](https://github.com/mihawk-99/PS5_RetroArch) | N64, GameCube/Wii, PSP, SNES, GBA, Mega Drive, arcade… | Mihawk | ✅ |

O Manta **não inclui** nenhum emulador, BIOS ou jogo: ele baixa os emuladores dos repositórios
oficiais de cada autor. BIOS e jogos são seus.

## Capturas

<p align="center">
  <img src="docs/screenshots/jogos.png" width="440" alt="Biblioteca de jogos">
  <img src="docs/screenshots/escolher-emulador.png" width="440" alt="Abrir em qual emulador?">
  <img src="docs/screenshots/detalhes.png" width="440" alt="Detalhes de um emulador">
  <img src="docs/screenshots/pastas.png" width="440" alt="Pastas e importação do pendrive">
  <img src="docs/screenshots/atualizacoes.png" width="440" alt="Atualizações">
  <img src="docs/screenshots/instalando.png" width="440" alt="Instalando um emulador">
</p>

## Requisitos

- **PS5 desbloqueado** com um HEN que libere o acesso a `/data` (etaHEN, LegacyJB ou OnionHEN,
  com **kstuff**).
- **[ShadowMountPlus](https://github.com/drakmor/ShadowMountPlus)**, que coloca na tela inicial
  os apps de `/data/homebrew/` (o Manta e os emuladores que ele instala).
- **Servidor FTP** no console (o do etaHEN ou o ftpsrv, porta 2121), só para instalar o Manta pela
  primeira vez.
- **Internet no PS5** para instalar/atualizar emuladores e baixar capas.
- Opcional: **ELF loader** na porta 9021 (vem no etaHEN/LegacyJB), usado para tirar da tela inicial
  o ícone de um emulador desinstalado.

Testado em: firmware 13.60, LegacyJB + kstuff + ShadowMountPlus.

## Instalação

Na página de [**Releases**](../../releases/latest) há dois jeitos de instalar. Escolha um.

### Opção 1 — instalador `.elf` (recomendado, sem FTP)

1. Baixe **`Manta-vX.Y.Z-installer.elf`**.
2. Envie o arquivo para o **ELF loader** do PS5 (porta **9021**), do mesmo jeito que você envia
   outros payloads:
   - do PC: `python scripts/send-elf.py <IP-do-PS5> Manta-vX.Y.Z-installer.elf`
     ou um app como o NetCat GUI;
   - ou pelo menu de payloads do seu HEN, se ele carregar `.elf` de um pendrive.
3. O PS5 avisa "Manta X.Y.Z instalado". Em instantes o **Manta** aparece na tela inicial.

O instalador já leva o app dentro dele: não precisa de internet. Se o Manta estiver aberto, ele
espera você fechar.

### Opção 2 — ZIP por FTP

1. Baixe **`Manta-vX.Y.Z-PPSA98265.zip`** e extraia. Você terá uma pasta chamada **`PPSA98265`**.
2. Com o FTP ligado, copie **a pasta inteira** para `/data/homebrew/` no console, para ficar
   `/data/homebrew/PPSA98265/eboot.bin`, `.../sce_sys/`, `.../assets/`.
   - **Cliente FTP** (FileZilla, WinSCP…): conecte em `IP-do-PS5`, porta `2121`, usuário anônimo,
     envie a pasta e dê permissão `777` a ela e a tudo dentro.
   - **Script** (Python 3): `python scripts/deploy-ps5.py <IP-do-PS5> --folder caminho/para/PPSA98265`
     envia tudo já com as permissões certas.
3. Espere alguns segundos: o ShadowMountPlus registra o app e o **Manta** aparece na tela inicial.

> O IP do PS5 está em **Configurações › Rede › Status da conexão**. Para conferir um download, o
> `SHA256SUMS.txt` da release traz o hash de cada arquivo (`certutil -hashfile <arquivo> SHA256`
> no Windows, `sha256sum` no Linux/macOS).

Na primeira vez, o Manta pede acesso a `/data` ao HEN e procura as versões mais novas dos
emuladores.

### Atualizar o Manta

**Pelo próprio Manta:** quando sai uma versão nova, ela aparece em **Atualizações** (linha
"Manta"). Aperte **Atualizar**. O Manta baixa e confere a versão nova, depois pede para você
fechar o app. Assim que você fecha, a troca é feita e o PS5 avisa "Manta atualizado para vX.Y.Z".
É só abrir de novo. Isso usa o ELF loader (porta 9021).

**Sem o ELF loader:** envie o instalador `.elf` novo, ou copie o ZIP novo por cima da pasta. Seus
emuladores, jogos e ajustes não são tocados.

### Desinstalar o Manta

Apague `/data/homebrew/PPSA98265` (e, se quiser, `/data/emuland`, onde ficam o cache de capas e os
ajustes). Os emuladores instalados continuam funcionando.

## Como usar

### Controles

| Botão | Ação |
|---|---|
| ✕ | Confirmar · Jogar · Instalar |
| ○ | Voltar |
| □ | Em Jogos: abrir as Pastas do emulador do jogo |
| L1 / R1 | Trocar de aba |
| OPTIONS | Ajustes |

### Passo a passo

1. **Instale um emulador** — Em **Início**, escolha o emulador e aperte ✕ para ver os detalhes.
   Aperte **Instalar**. O Manta baixa, confere e instala. No fim, **feche o Manta** quando ele
   pedir: o ShadowMountPlus só registra apps novos com o Manta fechado. Abra o Manta de novo.
2. **Prepare um pendrive** (exFAT ou FAT32) — Em **Pastas**, aperte **Preparar pendrive**. O
   Manta cria `Manta/<sistema>/jogos`, `Manta/<sistema>/bios` etc. e um `LEIA-ME.txt`.
3. **Copie seus arquivos no PC** — Jogos e BIOS nas pastas certas do pendrive. Exemplo:

   ```
   Manta/
   ├── ps1/
   │   ├── jogos/   Crash Bandicoot (USA).chd
   │   └── bios/    scph5501.bin
   └── ps2/
       ├── jogos/   Shadow of the Colossus (USA).iso
       └── bios/    (sua BIOS do PS2)
   ```

4. **Importe** — Conecte o pendrive no PS5. Em **Pastas**, escolha o emulador e aperte
   **Importar tudo do pendrive** (ou ✕ numa pasta para importar só ela).
5. **Jogue** — Em **Jogos**, escolha o jogo e aperte ✕. Se mais de um emulador roda aquele jogo,
   o Manta pergunta qual usar.
6. **Mantenha tudo em dia** — A aba **Atualizações** mostra o que tem versão nova. **Atualizar
   todos** resolve de uma vez. O Manta procura atualizações ao abrir (no máximo a cada 6 horas,
   para respeitar o limite do GitHub).

### Onde cada emulador guarda os arquivos

| Emulador | Pasta no console | Pasta no pendrive |
|---|---|---|
| PSXS5 | `/data/PSXS5` (`games`, `bios`, `covers`) | `Manta/ps1/` |
| SwanStationPS5 | `/data/SwanStationPS5` (`games`, `bios`, `covers`) | `Manta/ps1-swan/` |
| PS5SX2 | `/data/PCSX2` (`games`, `bios`, `cheats`, `patches`, `textures`, `memcards`) | `Manta/ps2/` |
| RPCS3 | `/data/rpcs3` (`games`) | `Manta/ps3/` |
| ProsperoEden | `/data/prosperoeden` (`roms`, `keys`, `firmware`, `updates`, `mods`) | `Manta/switch/` |
| PS5CEMU-HAR | `/data/ps5cemu` | `Manta/wiiu/` |
| XPSemu | `/data/xemu` | `Manta/xbox/` |
| RetroArch | dentro do app: `content`, `system`, `savefiles` | `Manta/retroarch/` |

A tela **Pastas** mostra sempre o caminho exato e o que está faltando.

## Perguntas frequentes

**O emulador diz "BIOS não encontrada", mas o jogo abre.** O SwanStationPS5 traz uma BIOS livre
(OpenBIOS) embutida, que roda a maioria dos jogos. Uma BIOS original melhora a compatibilidade:
coloque `scph5501.bin` (EUA), `scph5502.bin` (Europa) ou `scph5500.bin` (Japão) na pasta `bios`.

**Instalei um emulador e ele não aparece na tela inicial.** Feche o Manta: o ShadowMountPlus só
registra apps novos quando nenhum jogo/app está aberto.

**"Limite do GitHub: tente depois das HH:MM".** Sem conta, o GitHub aceita 60 consultas por hora
por IP. O Manta guarda a última resposta e avisa a hora em que dá para tentar de novo.

**O Manta apaga alguma coisa sem eu pedir?** Não. Importar só copia, atualizar guarda a versão
anterior até a nova estar no lugar, e desinstalar pergunta se você quer manter seus arquivos.

**Onde fica o log?** Em `/data/emuland/hub.log` (o da execução anterior em `hub.prev.log`). O
instalador `.elf` escreve em `manta-installer.log`, e a troca de versão e as desinstalações em
`manta-helper.log`, todos na mesma pasta. Anexe ao abrir uma issue.

**Quero sugerir um emulador para o catálogo.** Abra uma issue ou um pull request no
[`catalog/online.json`](catalog/online.json) ([formato](catalog/README.md)). Depois de aceito, ele
aparece para todos sem versão nova do Manta.

## Segurança

- Downloads só por **HTTPS**, só dos repositórios oficiais listados no catálogo, conferidos pelo
  **SHA-256** que o GitHub publica para cada arquivo.
- O ZIP é inspecionado antes de extrair: nada fora da pasta do app, sem caminhos com `..`, sem
  links simbólicos, limite de tamanho e de memória.
- O Manta só remove a pasta de um app depois de confirmar que ela é mesmo daquele app.
- Imagens (capas) com dimensões absurdas são recusadas antes de decodificar.
- O catálogo online passa por validação rígida: um emulador só pode usar uma pasta de dados
  própria em `/data/<nome>`, e um documento com qualquer regra quebrada é ignorado por inteiro.
- A troca de versão do próprio Manta só aceita a pasta que o Manta preparou e só grava na pasta
  do Manta. Se algo falhar no meio, a versão anterior volta.

Encontrou uma falha? Abra uma issue (ou, se for sensível, entre em contato pelo perfil do
mantenedor antes de divulgar).

## Compilar a partir do código

Precisa de **Windows com WSL (Ubuntu)** ou Linux, com `clang-18`, `lld-18`, `ninja-build`,
`rsync`, `python3`, `make` e `zip`.

```bash
# Windows
scripts\build-ps5.cmd                         # gera hub/dist/PPSA98265 e hub/dist/PPSA98265.zip
python scripts/deploy-ps5.py <IP-do-PS5>      # envia o build por FTP
python scripts/deploy-ps5.py <IP-do-PS5> --log   # traz /data/emuland/hub.log para logs/

# Linux / WSL
bash scripts/build-ps5.sh
```

O build gera também `hub/dist/manta-installer.elf` (o instalador de um passo, com o ZIP dentro).

**Publicar uma versão:** mude `MANTA_VERSION` em `hub/src/emuland/model.hpp` e `contentVersion` em
`hub/sce_sys/param.json` (um teste confere que batem), escreva a entrada do `CHANGELOG.md` (e, se
quiser, `docs/RELEASE-vX.Y.Z.md`), faça o commit e envie a tag:

```bash
git tag v1.4.0 && git push origin main v1.4.0
```

O GitHub Actions ([`release.yml`](.github/workflows/release.yml)) compila, roda os testes e publica
a release com o ZIP, o instalador `.elf` e o `SHA256SUMS.txt`. Os Mantas já instalados veem a
versão nova em Atualizações.

Na primeira vez, o build baixa o SDK de payloads do PS5, o ps5-opengl e o libcurl (PacBrew).

**Prévia no PC** (sem console): `wsl -- bash scripts/hub-preview.sh <pasta>` renderiza todas as
telas em PNG. **Testes:** dentro de `hub/`, `make test-unit` e `bash tools/installer-test.sh`.

### Estrutura

```
hub/        o app (C++20, OpenGL 4.6 pelo kit ps5-homebrew-ui)
  src/concepts/emuland.cpp      todas as telas
  src/emuland/                  catálogo, biblioteca de jogos, artes
  src/platform/ps5/             instalador, capas, lançador, acesso a /data
  host/                         versão de PC para prévia e testes
helper/     payloads: manta_helper (troca de versão, ícones de desinstalados) e o instalador .elf
catalog/    online.json (catálogo online) e emuladores.json (espelho do embutido)
.github/    build e publicação automáticos de cada versão
scripts/    build, envio por FTP e prévia
docs/       design (fonte de verdade da interface), plano, identidade visual
```

## Créditos

- Interface construída sobre o [ps5-homebrew-ui](https://github.com/blackbearreloaded/ps5-homebrew-ui)
  e o [ps5-native-app-boilerplate](https://github.com/blackbearreloaded/ps5-native-app-boilerplate),
  de BlackBearReloaded, com o [PS5 Payload SDK](https://github.com/ps5-payload-dev/sdk).
- Capas do [libretro-thumbnails](https://github.com/libretro-thumbnails/libretro-thumbnails).
- Fonte [Inter](https://github.com/rsms/inter); imagens com [stb](https://github.com/nothings/stb);
  ZIP com [miniz](https://github.com/richgel999/miniz); rede com libcurl + OpenSSL.
- Todos os autores dos emuladores do catálogo, sem os quais o Manta não teria o que reunir.

Avisos completos em [hub/THIRD_PARTY_NOTICES.md](hub/THIRD_PARTY_NOTICES.md).

## Licença

[GPL-3.0-or-later](LICENSE). PlayStation e PS5 são marcas da Sony Interactive Entertainment; este
projeto não tem relação com a Sony. Use apenas jogos e BIOS que você possui.
