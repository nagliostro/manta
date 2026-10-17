# Manta: design

Fonte de verdade de cores, fontes, componentes e telas. Proposta inicial, a aprovar na fase 1.

## Contexto

Interface de TV ("10 pés"): 1920×1080 desenhada, saída até 4K, vista do sofá e usada só com
DualSense. **Visual de serviço de streaming** (pedido do usuário em 14/10/2026): os emuladores
ocupam o lugar dos filmes e séries. Padrões seguidos: Netflix e Prime Video na TV (destaque grande
com a arte do item em foco, fileiras horizontais de cartões 16:9, a fileira em foco sobe e a de
cima some), Apple TV (abas em pílula no topo) e a loja do PS5 (L1/R1 trocam de aba).

## Identidade (16/10/2026)

O app se chama **Manta** (antes Emuland; a arraia-manta: calma, desliza, o mar azul-marinho da
paleta). Arquivos-fonte em [`docs/brand/`](brand/).

- **Logo:** um M desenhado pelo usuário, com o tema da manta: as hastes se curvam para fora no
  alto como pontas de asa, as diagonais são as asas, a cauda sobe ondulada pelo vão do meio e a
  cabeça com as duas nadadeiras fica no vértice de baixo. Vetor traçado do desenho
  (`manta-m.svg`); nada foi redesenhado.
- **Ícone do PS5** (`sce_sys/icon0.png`, 512×512): o M prateado (branco a `#cfd8e3`) com brilho
  suave azul-acinzentado, sobre degradê radial de `#3b4b60` a `#070a10`, com halo vindo de cima.
  Sem texto.
- **Capa da tela inicial** (`pic0.dds`/`pic1.dds`, 3840×2160 BC7, de `manta-capa.svg`): fundo do
  mar escuro com a superfície ondulada no alto, a rede de luz que ela projeta na água
  (cáusticas) e bolhas subindo atrás das mantas; uma manta grande desliza à direita, outra
  pequena e desfocada ao fundo, as duas com a cauda ondulada. O lado esquerdo fica livre.
- **No app:** no topo, o M (textura `assets/art/manta-mark.png`, 30 px de altura, pés na linha de
  base) seguido de "ANTA" em Inter SemiBold 24 com espaçamento 7,7, como as outras maiúsculas.
- **Texto:** "o Manta" (o app), para não confundir com manta, o cobertor.
- **Pendrive:** a pasta passa a ser `Manta/<sistema>/`; um pendrive com `Emuland/` continua
  valendo, para importar e para preparar (ele não é renomeado).
- Ficam como estavam, por serem internos: o Title ID PPSA98265, o `contentId` e a pasta de estado
  `/data/emuland`.

## Estética

- Minimalista e premium. **Só tema escuro**, como Netflix, Prime Video e a tela inicial do PS5 (o
  tema claro do protótipo anterior saiu: com as artes de fundo ele não funciona, e o PS5 não tem
  modo claro para seguir).
- **Cores:** fundo `#0B0E14` (quase preto frio), superfície `#151A23`, ardósia `#5B6878`,
  texto `#F2F4F7` (off-white), texto secundário `#9AA4B2`, acento azul-acinzentado claro `#A9C1DA`
  com texto escuro por cima.
- **Estado:** atualizado `#7FB69A` (verde sóbrio), atualização disponível = acento, falta arquivo
  `#D9B26F` (âmbar discreto). A interface do Emuland não usa roxo/lavanda.
- **Arte dos emuladores:** cada emulador aparece com o **ícone oficial** do projeto
  (`sce_sys/icon0.png`) como logotipo, e o **fundo oficial** da tela inicial do PS5
  (`sce_sys/background-source.png`) como arte do destaque e dos cartões. É conteúdo, como o
  pôster de um filme: mantém as cores de cada projeto (o roxo do PS5SX2 é dele, não do Emuland).
  O PS5SX2 só publica um banner largo com o nome escrito; o protótipo usa o trecho das ondas.
  Créditos em Ajustes > Sobre. No app, as artes vêm do pacote de cada emulador instalado ou do
  catálogo (campo a criar: `arte.icone`, `arte.fundo`).
- **Fonte:** Inter. Títulos 88px (destaque) e 64px (telas), texto 28px, mínimo 20px na TV.
- **Foco:** o item em foco cresce (6% nos botões, 8% nos cartões), ganha borda de 3px no acento e
  um brilho suave atrás. Nas abas do topo, o foco é a pílula preenchida em off-white (tvOS).
- **Legibilidade sobre a arte:** degradê do fundo para a esquerda e para baixo (onde ficam texto e
  fileiras), e um leve no topo (abas e relógio). Fora do Início a arte fica a 20%.
- **Botões:** pílula. Botão principal off-white com ▶ "Jogar". Rótulos de ação no rodapé, à
  direita, com os ícones do DualSense.

## Telas

### Início (estilo streaming)

```
┌───────────────────────────────────────────────────────────────────────────────┐
│ MANTA    (Início) Atualizações① Pastas Ajustes                        17:55   │
│                                                   ░░░ arte do emulador ░░░░░░ │
│ ┌────┐ PLAYSTATION 2                              ░░░░░░░░░░░░░░░░░░░░░░░░░░░ │
│ │icon│ PS5SX2                                     ░░░░░░░░░░░░░░░░░░░░░░░░░░░ │
│ └────┘                                            ░░░░░░░░░░░░░░░░░░░░░░░░░░░ │
│ vk-284-170 · Swordpdf · GPL-3.0                                               │
│ (↓ Atualização disponível) (✓ BIOS ok) (38 jogos)                             │
│ O PCSX2 rodando nativo no PS5, com recompiladores…                            │
│ (▶ Jogar) (Atualizar) (Pastas) (Detalhes)                                     │
│                                                                               │
│ Seus emuladores                                                               │
│ ┌────────┐ ╔════════╗ ┌────────┐ ┌ ─ ─ ─ ─┐                                   │
│ │ PSXS5  │ ║ PS5SX2 ║ │ Eden   │   + Mais                                     │
│ └────────┘ ╚════════╝ └────────┘ └ ─ ─ ─ ─┘                                   │
│ Novidades                     ✕ Selecionar △ Detalhes □ Pastas L1R1 Menu      │
└───────────────────────────────────────────────────────────────────────────────┘
```

- **Destaque (billboard):** ícone, sistema, nome, versão/autor/licença, três selos de estado,
  descrição em até duas linhas e ações. Arte e textos trocam com o cartão em foco (fundo em
  transição de 0,6 s).
- **Fileiras** (app nativo, com o catálogo de 8 emuladores):
  1. *Seus emuladores*: os instalados, mais o cartão "+ Mais emuladores", que leva à primeira
     fileira do catálogo. ✕ joga.
  2. *Novidades*: os instalados com atualização. ✕ atualiza. Some quando não há nada.
  3. *Sony · PlayStation*, *Nintendo*, *Microsoft · Xbox*, *Vários sistemas*: o catálogo
     inteiro por fabricante, como os gêneros da Netflix. Selo "Não instalado" (arte esmaecida) ou
     "Só código-fonte". ✕ abre os Detalhes, onde fica o botão Instalar (nunca instala direto).
  4. *Pastas importantes*: jogos e arquivos obrigatórios dos instalados, com o estado.
  Num emulador não instalado o destaque mostra o tamanho do download, não o estado das pastas.
- **Navegação:** ←/→ dentro da fileira (ela rola sozinha depois do 4º cartão), ↑/↓ entre
  fileiras lembrando a posição de cada uma. Ao descer da primeira fileira, a descrição e os botões
  do destaque somem e a fileira em foco sobe. ↑ da primeira fileira vai ao botão principal;
  ↑ dos botões vai às abas. ○ numa fileira de baixo volta à primeira.

### Abas

Início · Jogos · Atualizações (com contador) · Pastas · Ajustes. L1/R1 trocam de aba em qualquer tela;
OPTIONS vai a Ajustes; ○ numa aba volta ao Início. Detalhes abre por cima do Início.

### Jogos (biblioteca)

Aprovada em 16/10/2026. Padrão seguido: a
Biblioteca de jogos do PS5 (filtro + grade de capas) e os pôsteres de texto do app TV da Apple.

```
┌───────────────────────────────────────────────────────────────────────────────┐
│ MANTA    Início (Jogos) Atualizações Pastas Ajustes                    21:40   │
│ PLAYSTATION 2 · PS5SX2                                                        │
│ Gran Turismo 4                                                                │
│ 7,7 GB · ISO · EUA                                                            │
│ (Todos 26) (◉ PS1 12) (◉ PS2 14)                                              │
│ ┌────┐ ┌────┐ ╔════╗ ┌────┐ ┌────┐ ┌────┐ ┌────┐                             │
│ │PS2 │ │PS1 │ ║PS2 ║ │PS2 │ │    │ │    │ │    │   7 capas por fileira       │
│ │God │ │GT 2│ ║GT 4║ │GTA │ │Ico │ │Jak │ │KH2 │                             │
│ └────┘ └────┘ ╚════╝ └────┘ └────┘ └────┘ └────┘                             │
│ ┌ ─ ─┐ ┌ ─ ─┐  (a fileira de baixo só aparece pela metade)                    │
│                                   ✕ Jogar  □ Pastas  L1R1 Menu  ○ Voltar      │
└───────────────────────────────────────────────────────────────────────────────┘
```

- **O que entra:** os arquivos da pasta de jogos de cada emulador instalado, em ordem alfabética.
  O nome vem do arquivo, como o No-Intro/Redump o escreve: sem extensão, sem etiquetas
  `(USA)`/`[SLUS-…]`, e " - " vira ": " (`Castlevania - Symphony of the Night (USA).chd` →
  "Castlevania: Symphony of the Night"). A primeira região vira "EUA", "Europa", "Japão"...
- **Topo:** o que está em foco. Num jogo: sistema · emulador, título em 64 px, tamanho · formato ·
  região. Num filtro: "BIBLIOTECA", "Todos os jogos" ou o sistema, e quantos jogos.
- **Filtros:** pílulas "Todos" e uma por emulador com jogos (ícone, sigla e contagem), no estilo
  das de Pastas. ✕ troca o filtro; a grade entra com o mesmo deslize de Pastas.
- **Capas:** 224×312 (proporção de caixa de DVD), cantos de 16 px, 7 por fileira. Uma capa de
  caixa (PS2, Xbox, Wii U) ocupa o cartão; uma de outro formato (a caixa quadrada do PS1) aparece
  inteira no alto, com o título embaixo. Sem capa, um pôster de texto: o meio
  da arte do emulador escurecida, o ícone e a sigla no alto e o título em até 4 linhas embaixo.
  O foco cresce 7% com o anel de acento e o brilho atrás.
- **Navegação:** por posição, como no PS5: ←/→ na fileira, ↑/↓ mantêm a coluna (a última
  fileira curta pega a última capa), ↑ da primeira fileira vai ao filtro em uso, ↑ dele às abas.
  A grade rola e a fileira em foco fica logo abaixo dos filtros; a de cima some, a de baixo
  aparece pela metade e sem título.
- **Ações:** ✕ Jogar abre o emulador, com a tela "Abrindo no PS5SX2" e o título do jogo em
  destaque. □ abre Pastas daquele emulador (○ volta a Jogos). O jogo abre direto nos emuladores
  que aceitam `--rom <caminho>` ao abrir (SwanStationPS5 e PS5CEMU-HAR, campo `rom_option` do
  catálogo); o Emuland passa o par duas vezes, porque não se sabe se o sistema põe o nome do
  programa em argv[0]. PSXS5, PS5SX2 e ProsperoEden não leem argumentos (conferido no código em
  16/10/2026): abrem no menu, e a tela de abertura diz "Escolha o jogo no menu do PS5SX2". O
  RetroArch tem um "game mode" próprio, por arquivo de pedido, ainda não usado.
- **Mais de um emulador para o jogo** (como o "Abrir com" do Android): se outro emulador
  instalado do mesmo sistema aceita `--rom`, ✕ abre o diálogo "Abrir em qual emulador?" com
  "“<jogo>” roda em mais de um emulador instalado." e um botão por emulador (o dono da pasta
  primeiro), mais Cancelar. Só entra quem consegue achar o jogo: o dono da pasta e os que recebem
  o caminho. Um jogo de PS1 do PSXS5 pergunta (PSXS5 ou SwanStationPS5); um da pasta do
  SwanStationPS5 abre direto, porque o PSXS5 só lista a própria pasta. O foco começa no emulador
  escolhido da última vez para jogos daquela pasta (lembrado só enquanto o Manta está aberto).
- **Vazia:** "Nenhum jogo ainda", como copiar para `Emuland/<sistema>/jogos` no pendrive ou HD e
  importar, e o botão "Abrir Pastas". Sem emulador instalado, manda instalar um no Início.
- A arte do fundo segue o emulador do jogo em foco, a 62% de escurecimento.
- **De onde vêm as capas** (em segundo plano, `src/platform/ps5/covers.cpp`): 1) a pasta de capas
  do emulador (as suas, e as que o PSXS5 baixa); 2) o cache do Emuland,
  `/data/emuland/covers/<sistema>/<arquivo>.png`; 3) com "Baixar capas dos jogos" ligado (Ajustes,
  ligado por padrão), o libretro-thumbnails (`thumbnails.libretro.com`, as capas do RetroArch). A
  lista de nomes de cada sistema fica guardada 30 dias; vale o mesmo título, depois a mesma região.
  Sistemas com capas lá: PS1, PS2, PS3, Wii U e Xbox. Switch e o RetroArch (pasta com vários
  sistemas) ficam no pôster de texto. Cada capa é reduzida a 256 px de largura antes de virar
  textura, duas por quadro. Créditos em Ajustes > Sobre.
- **Jogos no console:** cada arquivo da pasta de jogos com as extensões do emulador, ou cada
  subpasta que tenha um (uma pasta por jogo). Discos e faixas do mesmo jogo (mesmo título e
  região) aparecem uma vez, pelo `.m3u`, senão pelo `.cue`. O contador de jogos do Início usa a
  mesma conta.

### Detalhes do emulador

Descrição, autor, licença, link do repositório (QR), requisitos com marca de ok/falta, notas da
última versão, botões Instalar / Atualizar / Voltar versão / Desinstalar.

### Pastas

Uma linha por pasta do emulador: nome, caminho, estado ("3 arquivos", "Falta prod.keys") e ação
"Importar do pendrive". Topo com o layout esperado no pendrive.

### Atualizações

Lista dos emuladores com versão instalada → nova, tamanho, "Atualizar todos". Progresso em anel com
tempo restante; ○ cancela.

### Ajustes

Idioma (segue o do console), baixar capas dos jogos, procurar atualizações ao abrir,
pré-releases, sobre/créditos.

## Protótipo

[`prototype/index.html`](../prototype/index.html): abre direto no navegador, sem build. Palco fixo de
1920×1080 escalado para a janela. Dados de exemplo; nada é baixado nem copiado de verdade. As artes
ficam em `prototype/assets/` (cópias reduzidas das oficiais: ícone 256 px, fundo 1920×1080).

- **Controles:** setas movem; Enter/A = ✕, Esc/B = ○, Y = △, X = □, Q/E = L1/R1 (abas),
  O = OPTIONS, H = atalhos. Controle no computador pela Gamepad API, e mouse.
- **Capturas:** `index.html?keys=down,down` simula botões ao abrir (ex.: `?keys=r1` abre
  Atualizações, `?keys=t` os Detalhes).
- **Estados de exemplo:** PSXS5 atualizado, PS5SX2 com atualização disponível e ProsperoEden não
  instalado, para mostrar os três caminhos.
- **Decisões:**
  - Botão principal em off-white com texto escuro. O acento fica no anel de foco, nos selos de
    atualização e no progresso.
  - Detalhes: ícone + nome no topo, a arte do emulador ao fundo; ações, destaques e requisitos à
    esquerda; versão à direita.
  - Pastas: seletor de emulador em pílulas com o ícone de cada um (substitui L1/R1, que agora
    trocam de aba). Uma linha por pasta; à direita, o layout do pendrive e as ações.
  - Ajustes em grade de duas colunas, sem rolagem. ✕ alterna o valor. Sem opção de tema.
  - Tela "Abrindo": a arte do emulador desfocada ao fundo, ícone grande e nome.
  - Desinstalar pede confirmação e lembra que jogos, saves e BIOS continuam no console.
  - "Voltar versão" só aparece depois que o Emuland fez uma atualização (ele guarda a anterior).

## No app nativo (`hub/`)

O visual do protótipo roda no PS5 sobre o kit
[ps5-homebrew-ui](https://github.com/blackbearreloaded/ps5-homebrew-ui) (OpenGL 4.6, um shader
SDF para formas, texto e imagens; já medido a 60 fps em 4K no console). O design é um arquivo,
[`hub/src/concepts/emuland.cpp`](../hub/src/concepts/emuland.cpp), com as mesmas medidas do
protótipo no canvas virtual de 1920×1080.

- **Fontes:** Inter Regular, SemiBold e Bold, geradas com o conjunto `european` (acentos do
  português). A Bold ocupa o lugar "display" do kit (títulos de 64 e 88 px).
- **Movimento:** molas do kit. O anel de foco desliza entre cartões, botões e linhas; a arte do
  fundo troca em 0,6 s; o destaque entra com um leve deslize; as fileiras sobem quando o foco
  desce; uma fileira na ponta recusa com um tremor curto. "Reduzir movimento" encurta tudo.
- **Sons:** o conjunto "glass" do kit (foco, aba, abrir, voltar, erro, aviso). Desligáveis em
  Ajustes > Sons da interface.
- **Barra de luz do controle:** azul-marinho `#1E3A5F`.
- **Prévia no PC:** `wsl -- bash scripts/hub-preview.sh` gera PNGs de todas as telas (o
  roteiro fica em `kTour`, no próprio design); `EMULAND_LANG=en` mostra o inglês.
- **Instalar e atualizar (GitHub):** ao abrir, o Emuland procura o release mais novo de cada
  emulador (em segundo plano; a tela mostra a última resposta guardada). Sem conta, o GitHub
  aceita só 60 consultas por hora, uma por emulador: a busca ao abrir só vai ao GitHub se a
  última completa tiver mais de 6 horas, os instalados vêm primeiro, e um limite atingido para a
  busca até o horário que o GitHub informa ("Limite do GitHub: tente depois das 15:42" em
  Atualizações; "Procurar agora" mostra o mesmo em um diálogo). As versões já conhecidas
  continuam valendo. Instalar, Atualizar e
  "Atualizar todos" abrem uma janela de progresso com um anel, as etapas (procurar o release,
  baixar com MB, conferir SHA-256, extrair, instalar) e ○ Cancelar. No fim, um aviso "PS5SX2
  vk-285-171 instalado"; um erro abre um diálogo que explica o que houve em linguagem simples
  (sem conexão, emulador aberto, arquivo diferente do publicado...) e o detalhe técnico abaixo.
- **Versões:** a tela mostra a tag do release instalado pelo Emuland (v2.2.0, vk-285-171). Uma
  cópia instalada à mão mostra a `contentVersion`; como PSXS5 e PS5SX2 não mudam a
  `contentVersion` a cada release, a primeira atualização pelo Emuland é oferecida e, a partir
  dela, a comparação é exata.
- **Importar do pendrive:** ✕ numa linha de Pastas importa só aquela pasta; "Importar tudo do
  pendrive" importa todas. O Manta procura `Manta/<sistema>/<pasta>/` (ou `Emuland/`, de antes do nome novo) em `/mnt/usb0` a
  `/mnt/usb7` (exFAT ou FAT32) e copia para a pasta de dados do emulador, mantendo subpastas
  (uma por jogo). Só copia: nada no pendrive muda e nada no console é apagado. Um arquivo que já
  está no console com o mesmo tamanho é pulado, então importar de novo continua de onde parou.
  Arquivos ocultos (`._*` e `.DS_Store` do macOS) ficam de fora. Cada arquivo é gravado como
  `.emuland-part` e só ganha o nome no fim. A janela mostra "Importando para PSXS5", o caminho
  `Emuland/ps1/ → /data/PSXS5`, o anel com % e MB, e ○ Cancelar. No fim, "3 arquivos importados"
  ou "Nada novo no pendrive". Erros: "Pendrive não encontrado", "Sem espaço no console" ou
  "Não foi possível copiar".
- **Pendrive ou HD externo:** os dois montam em `/mnt/usb0` a `usb7` (exFAT ou FAT32; o PS5 não
  lê NTFS). Essas pastas existem vazias mesmo sem nada conectado, então só conta como disco a
  que tem outro dispositivo montado (`st_dev` diferente de `/mnt`): sem isso, o Emuland gravaria
  dentro do sistema.
- **Preparar pendrive:** terceiro botão de Pastas. No disco que já tem `Emuland/` (senão, no
  primeiro conectado), cria `Emuland/<sistema>/` só dos emuladores instalados, com todas as
  pastas de cada um, e um
  `Emuland/LEIA-ME.txt` (`README.txt` em inglês) que lista o que vai em cada pasta. Sem nenhum
  emulador instalado, avisa "Instale um emulador antes de preparar o pendrive". Ao instalar outro,
  preparar de novo cria só as pastas dele. Não mexe em
  nada que já esteja no pendrive. Aviso "31 pastas criadas no pendrive" ou "O pendrive já tem
  todas as pastas"; sem pendrive ou sem gravação, um diálogo explica. Roda em segundo plano:
  enquanto isso o botão diz "Preparando" com pontos que contam até três e uma luz branca percorre
  o anel de foco em volta da pílula (no mínimo 1,2 s, para não piscar num pendrive rápido; com
  "Reduzir movimento", só o texto).
- **Instalar a partir dos Detalhes:** ✕ num emulador não instalado (cartão, Novidades ou botão
  Instalar do destaque) abre os Detalhes; só o Instalar de lá começa o download. Atualizar continua
  direto.
- **Desinstalar:** último botão dos Detalhes, só com o emulador instalado. Abre o diálogo
  "Desinstalar PS5SX2?" com "Manter meus arquivos" (foco inicial), "Apagar tudo" e "Cancelar".
  Manter apaga só o app; Apagar tudo leva também a pasta de dados (jogos, BIOS, saves, ajustes).
  O RetroArch guarda os dados dentro do app: Manter deixa lá `content`, `system` e `savefiles`.
  A janela de progresso mostra "Desinstalando" com o anel girando, sem Cancelar.
  O ícone da tela inicial só sai com o Emuland fechado: ele mostra "Feche o Emuland para concluir"
  e deixa um payload esperando (`helper/uninstall_helper.c`, enviado à porta 9021), que pede ao
  ShadowMountPlus para tirar o ícone assim que o Emuland fecha, com um aviso do PS5.
- **Registro no PS5 (ShadowMountPlus):** ele só registra apps novos, ou tira o ícone de um
  desinstalado, com o Emuland fechado. Depois de instalar, o selo vira "Registro pendente" e um
  diálogo "Feche o Emuland para concluir" oferece "Fechar o Emuland" ou "Depois".
- **Diferenças do protótipo:** sem "Voltar versão" por enquanto; Ajustes ganhou "Sons da interface" e "Sair do Emuland"; a árvore do pendrive
  usa `|--` (a fonte mono tem só ASCII).

## Acessibilidade

Contraste AA em todo texto, foco sempre visível, nenhuma informação só por cor (selos têm ícone e
texto), opção de reduzir movimento.
