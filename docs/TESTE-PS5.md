# Teste 1 no PS5

Primeiro teste do Emuland no console. A interface é a mínima do template (fonte de bitmap, sem
acentos) e serve para saber o que funciona no seu PS5. Não é o visual final.

## O que o teste verifica

| Item | Como aparece na tela |
|---|---|
| O app abre pela tela inicial | O ícone "E / EMULAND" e o nome **Emuland** |
| DualSense | Barra de luz azul-marinho, linha `CONTROLE: OK`, `BOTOES` mudando ao apertar |
| Navegação | Direcional ou analógico esquerdo sobem e descem na lista |
| Ler `/data` | Cada emulador mostra `INSTALADO Vxx.xxx.xxx`, `NAO INSTALADO` ou `SEM ACESSO A /DATA (código)` |
| Listar `/data/homebrew` | `LISTAR /DATA/HOMEBREW: OK, N ITENS` ou `FALHA código` |
| Gravar log | `LOG: /DATA/EMULAND/TESTE.LOG` (ou `/DOWNLOAD0` se `/data` estiver fechado) |
| Abrir um emulador | ✕ no emulador: `ABRIR PPSA...: código` e o emulador deve abrir |

Controles: direcional/analógico movem, ✕ abre, △ atualiza a lista, OPTIONS fecha o Emuland.

## Requisitos no console

- PS5 desbloqueado com **etaHEN + kstuff** (ou equivalente), como pedem PSXS5 e ProsperoEden.
- **ShadowMountPlus** carregado (é ele que põe apps de `/data/homebrew/` na tela inicial).
- Servidor **FTP** ativo (o do etaHEN fica na porta 2121).
- Para testar "abrir emulador", pelo menos um dos três instalado em `/data/homebrew/`.

## Passo a passo

1. **Compilar** (já está compilado em `app/dist/PPSA98265/`; só refaça se mudar o código):
   `scripts\build-ps5.cmd`
2. **Enviar ao PS5** (troque pelo IP do console, visto em Configurações > Rede):
   `python scripts\deploy-ps5.py <IP-do-PS5>`
   Sem o script: copie a pasta `app\dist\PPSA98265` inteira para `/data/homebrew/` pelo FileZilla
   e dê permissão `777` recursiva (botão direito > Permissões de arquivo).
3. Espere o ShadowMountPlus mostrar o **Emuland** na tela inicial (ou reinicie o ShadowMountPlus).
4. Abra, teste os itens da tabela e feche com OPTIONS.
5. **Trazer o log:** `python scripts\deploy-ps5.py <IP-do-PS5> --log` salva em `logs\teste.log`.
   Se o log foi para `/download0`, mande uma foto da tela.

## O que me mandar

O `logs\teste.log` (ou foto da tela), o firmware do PS5 e o que aconteceu ao apertar ✕ em um
emulador instalado (abriu, apareceu um aviso, ou nada).

## Resultados

**Teste 1 (14/10/2026, firmware 13.60, LegacyJB + ShadowMountPlus):** o app abriu, desenhou a
tela e leu o controle. Ao apertar ✕ no PSXS5, travou 6 s depois de abrir (log do
ShadowMountPlus: `crash-candidate flags=0x00088102`); o PSXS5 não chegou a iniciar. O log não
foi criado em `/data/emuland`: o app rodava na sandbox, sem permissão para criar pastas em
`/data`.

**Teste 2 (versão 01.000.001):** o app pede acesso a `/data` ao LegacyJB/etaHEN ao abrir, do
mesmo jeito que o PSXS5 (`/download0/etahen_jailbreak` com o PID e, sem resposta, o comando na
porta 9028). O resultado aparece como `/DATA: ...` na tela. Antes de chamar
`sceLncUtilLaunchApp`, grava um marcador: se o app travar, a próxima abertura mostra
`A ULTIMA TENTATIVA DE ABRIR ... FECHOU O EMULAND`. O log da execução anterior vai para
`/data/emuland/anterior.log`.

Resultado: o LegacyJB liberou o app ("Granted jailbreak"), `/data` ficou acessível e o app leu
a versão instalada dos três emuladores (PSXS5 01.000.000, PS5SX2 02.000.000, ProsperoEden
01.001.000). `sceLncUtilLaunchApp` continuou fechando o app, mesmo liberado. Depois da liberação,
`/download0` deixa de existir para o app, e o marcador gravado lá se perdeu.

**Teste 3 (versão 01.000.002):** ✕ usa `sceSystemServiceLaunchApp` (a função pública para um app
abrir outro, no PS4), com o mesmo bloco de parâmetros. O marcador e o log anterior ficam em
`/data/emuland`. A listagem de `/data/homebrew` usa `opendir`/`readdir`, como o PSXS5.

Resultado: **funcionou.** O ProsperoEden (PPSA99008) abriu a partir do Emuland. A chamada
devolveu `0x80940010` (não zero) e mesmo assim o emulador abriu: o app não deve tratar esse
código como falha. A listagem de `/data/homebrew` passou a funcionar (8 itens).

### O que ficou provado no console (firmware 13.60)

| Item | Como |
|---|---|
| App nativo na tela inicial | Template ps5-native-app-boilerplate + ShadowMountPlus |
| DualSense | `scePadOpen`/`scePadRead`, barra de luz com `scePadSetLightBar` |
| Acesso a `/data` | Pedido ao LegacyJB por `/download0/etahen_jailbreak` (método do PSXS5) |
| Ler versão instalada | `/data/homebrew/<TID>/sce_sys/param.json` |
| Abrir emulador | `sceSystemServiceLaunchApp(titleId, NULL, &param)`; `sceLncUtilLaunchApp` fecha o app |
| Envio e log | `scripts/deploy-ps5.py` por FTP; log em `/data/emuland/teste.log` |

## Teste 4: o visual novo (`hub/`)

O app com o visual de streaming. Mesmo title ID (PPSA98265), versão 01.001.000: substitui o app
de teste na tela inicial.

1. Compilar: `scripts\build-ps5.cmd` (o padrão agora é `hub/`; para o app de teste, rode
   `set APP=app` antes).
2. Enviar: `python scripts\deploy-ps5.py <IP-do-PS5>`.
3. O ShadowMountPlus guarda uma cópia do `param.json` e das artes da tela inicial na primeira vez;
   se o Emuland continuar mostrando a versão antiga, reinicie o ShadowMountPlus.
4. Abrir e conferir: fundo com a arte do emulador em foco, fileiras, abas com L1/R1, Detalhes
   (△), Pastas (□, com a contagem real de arquivos), "Criar pastas que faltam", Ajustes, e ✕ em
   um emulador instalado abre o emulador.
5. Log: `python scripts\deploy-ps5.py <IP-do-PS5> --log` traz `logs\hub.log` (e o da execução
   anterior). Ele traz o resultado do pedido de `/data`, as versões lidas, a quantidade de artes e
   sons carregados, o tempo por quadro a cada 30 s e o código de cada abertura de emulador.

O que só o console mostra: taxa de quadros (o app abre em 4K), som, a sensação do controle e se
`/app0` ainda existe depois da liberação (o app procura os arquivos em
`/data/homebrew/PPSA98265` quando não existe).

**Resultado do teste 4 (14/10/2026, PS5 em <IP-do-PS5>):** o visual novo abriu. O log mostra:
- `/data` liberado ("LIBERADO");
- versões lidas: PSXS5 01.000.000, PS5SX2 02.000.000, ProsperoEden 01.002.000;
- `/app0` sumiu depois da liberação, e os arquivos foram lidos de
  `/data/homebrew/PPSA98265/assets`, como previsto;
- tela em 4K (3840×2160) com OpenGL 4.6 ("PS5 AGC"), 6 artes e 67 sons carregados, controle
  aberto, primeiro quadro desenhado;
- memória de cerca de 31 MB em uso, sem falhas.

O log para no primeiro quadro: o resto ficou no buffer quando o app foi fechado pelo botão PS (o
resumo de desempenho só sai a cada 30 s).

## Teste 5: voltar ao Emuland a partir do emulador (14/10/2026)

Payload de sondagem [`helper/pad_probe.c`](../helper/pad_probe.c), enviado com
`python scripts/send-elf.py <IP-do-PS5> helper/pad_probe.elf` (ELF loader na porta 9021, que o
console aceitou).

- **Ler o controle em segundo plano: não funciona.** `scePadOpen` = `0x809B0081` (também depois de
  `scePadSetProcessPrivilege(1)`, que devolveu 0), `scePadGetHandle` = `0x80920008`,
  `scePadOpenExt` = `0x80920001`, `scePadOpen` com o usuário do sistema = `0x809B0001`. Um
  atalho de botões dentro do emulador fica descartado por esse caminho.
- **Vigiar o app da frente e trazer o Emuland de volta: funciona.** O payload viu PSXS5
  (PPSA97510) sair da frente e chamou `sceLncUtilLaunchApp(PPSA98265)` = `0x00004018`; o Emuland
  voltou em cerca de 1 s. Log em `logs/padprobe.log`.
- O usuário ainda não decidiu se quer esse comportamento; o recurso ficou em espera.

## Teste 6 (pendente): instalar e atualizar pelo GitHub

Versão 01.002.000 do `hub/` (libcurl + OpenSSL do PacBrew; o app está liberado, e a biblioteca
HTTPS do sistema recusa certificados nesse caso). Já testado no PC com downloads reais
(`hub/tools/installer-test.sh`: os três emuladores, troca, backup, 0777, recusa com o emulador
aberto).

1. Enviar: `python scripts\deploy-ps5.py <IP-do-PS5>`. Abrir o Emuland.
2. Alguns segundos depois de abrir, o aviso "N atualizações disponíveis" (se houver) e o contador
   na aba Atualizações. A aba mostra "Última verificação: hh:mm".
3. Atualizações > Atualizar em um emulador: janela com anel, MB baixados e etapas; no fim, o
   aviso "<nome> <tag> instalado". Abrir o emulador pelo Emuland para ver se a versão nova roda.
4. Testar ○ Cancelar durante um download: aviso "Cancelado. Nada foi alterado."
5. Log: `python scripts\deploy-ps5.py <IP-do-PS5> --log` (`logs\hub.log`: cada release
   encontrado, cada instalação e o motivo de qualquer falha).

O que só o console responde: se o curl conecta ao GitHub liberado (o template e o ProsperoRadio
mostraram que sim), a velocidade do download e se a troca dentro de `/data/homebrew` funciona
com o ShadowMountPlus montando a pasta.

### Resultado parcial (14/10/2026, em pausa)

- A verificação no GitHub funciona no console e encontra os 7 releases.
- PS5CEMU-HAR, 1ª tentativa: "curl 42". A janela de instalação estava mostrando a verificação
  automática, e o ○ cancelou as duas. Corrigido: a instalação passa na frente da verificação, e o ○
  não cancela mais durante a etapa "Instalando".
- 2ª tentativa: o download e a extração terminaram (`.emuland-new-PPSA99360` completa), mas o
  processo morreu antes de mover a pasta para o lugar, sem deixar log.
- 3ª tentativa: o console inteiro parou de responder (sem ping nem FTP), provavelmente travou ou
  reiniciou.
- Suspeita: o ShadowMountPlus vigia `/data/homebrew` e pega a pasta temporária, que tem
  `sce_sys/param.json`.
- Próximo passo: extrair em `/data/emuland/` e só mover para `/data/homebrew` no fim. Antes,
  confirmar o que apareceu na tela e o que sobrou em `/data/homebrew`. O log agora grava cada etapa
  na hora.

## Detalhes técnicos

- Title ID `PPSA98265`, `contentId UP9000-PPSA98265_00-EMULANDHUB000001`, versão `01.000.000`.
- Base: [ps5-native-app-boilerplate](https://github.com/blackbearreloaded/ps5-native-app-boilerplate)
  (commit `b8dc4a3`), firmware verificado pelo template: 6.02 e 12.70.
- Controle: `scePadInit` / `scePadOpen` / `scePadRead`, sequência da documentação do template
  (`docs/RECIPES.md`, "Controller input"); layout do `ScePadData` do ProsperoEden.
- Abrir emulador: `sceLncUtilLaunchApp(titleId, NULL, &param)`, como o PS5SX2 faz em
  `ps5/disclaunch/disc_launch.c`. Do PS5SX2 só há registro de chamadas feitas por um payload;
  feita por um app, é exatamente o que este teste vai mostrar.
- Fechar: `sceSystemServiceLoadExec("exit", NULL)`, a saída que o template recomenda.
- Fundos da tela inicial (`pic0.dds`, `pic1.dds`) e o som ainda são os do template.
