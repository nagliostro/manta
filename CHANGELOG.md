# Changelog

Todas as mudanças relevantes do Manta. O formato segue o
[Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/) e as versões seguem o
[Versionamento Semântico](https://semver.org/lang/pt-BR/): `MAIOR.MENOR.CORREÇÃO`. A versão do app
fica em `hub/src/emuland/model.hpp` (`MANTA_VERSION`) e em `contentVersion` de
`hub/sce_sys/param.json` (`1.3.0` = `01.003.000`); um teste confere que as duas batem.

## [1.4.0] - 2026-10-17

### Adicionado
- **Instalador de um passo** (`Manta-vX.Y.Z-installer.elf`): enviado ao ELF loader (porta 9021), instala
  ou atualiza o Manta sem FTP e sem internet, com o app embutido.
- **Autoatualização**: o próprio Manta aparece em Atualizações. Ele baixa e confere a versão nova, e
  o payload `manta_helper` faz a troca assim que o Manta fecha, voltando à versão anterior se algo
  falhar.
- **Catálogo online** (`catalog/online.json`): emuladores novos, e correções de onde um emulador é
  baixado, chegam sem versão nova do app. O arquivo passa por validação rígida.
- **Publicação automática**: uma tag `vX.Y.Z` no GitHub compila, testa e publica a release com o
  ZIP, o instalador `.elf` e o `SHA256SUMS.txt`.

### Mudado
- O payload `uninstall_helper.elf` virou `manta_helper.elf`, que também aplica a autoatualização.

## [1.3.0] - 2026-10-17

Primeira versão pública.

### Adicionado
- **"Abrir em qual emulador?"**: na biblioteca de Jogos, quando mais de um emulador instalado roda
  o jogo (ex.: PSXS5 e SwanStationPS5 para PS1), o Manta pergunta qual usar e lembra da última
  escolha enquanto está aberto.
- `scripts/deploy-ps5.py --folder <pasta>` instala no console uma release baixada, sem compilar.
- Testes do Manta (catálogo, nomes de jogos, versões, limites de imagem) e um caso novo no teste
  do instalador.

### Segurança
- Desinstalar e atualizar só mexem numa pasta indicada pelo `mount.lnk` do ShadowMountPlus se ela
  for mesmo do emulador (o `param.json` dela tem o title ID). Antes, um registro errado podia levar
  o Manta a apagar outra pasta, como um pendrive inteiro.
- Caminhos de `mount.lnk` com `..` são ignorados.
- Imagens (capas baixadas ou da pasta do usuário) com mais de 4096 px de lado são recusadas antes
  de decodificar, evitando que um arquivo pequeno esgote a memória do app.
- O nome do arquivo de uma release do GitHub não pode conter `/`, `\` nem `..`.

### Corrigido
- A contagem de jogos em subpastas tinha recursão sem limite: um link apontando para a pasta de
  cima podia derrubar o app.
- Um `idioma.txt` inválido em `/data/emuland` podia gerar um índice negativo.
- A versão mostrada em Ajustes e enviada ao GitHub (User-Agent) vinha de textos fixos e
  divergentes; agora vem de uma constante só.
- Num computador novo, o build gerava o app sem o payload de desinstalação (o SDK era baixado
  depois). O script agora baixa o SDK antes.
- O aviso do PS5 depois de uma desinstalação ainda dizia "Emuland".

## Antes da 1.3.0 (14 a 16/10/2026)

Versões de teste internas, só no console de desenvolvimento, sem release. O que veio delas:

- Primeiro app nativo: catálogo, instalação e atualização pelo GitHub com SHA-256, pastas de cada
  emulador e abertura pelo title ID.
- Nova identidade: **Manta** (antes Emuland), com ícone, capa da tela inicial e logo novos.
- Biblioteca de **Jogos** com capas (pasta do emulador, cache e libretro-thumbnails), filtro por
  sistema e pôsteres de texto para jogos sem capa.
- Jogo aberto direto nos emuladores que aceitam `--rom` (SwanStationPS5, PS5CEMU-HAR).
- **Desinstalar** (manter ou apagar os arquivos), com o payload que tira o ícone da tela inicial
  quando o Manta fecha.
- **Importar do pendrive** e **Preparar pendrive**.
- Catálogo com 8 emuladores: PSXS5, SwanStationPS5, PS5SX2, RPCS3, ProsperoEden, PS5CEMU-HAR,
  XPSemu e RetroArch.

[1.4.0]: https://github.com/nagliostro/manta/releases/tag/v1.4.0
[1.3.0]: https://github.com/nagliostro/manta/releases/tag/v1.3.0
