# Catálogo do Manta

| Arquivo | Para quê |
|---|---|
| [`online.json`](online.json) | **Catálogo online.** O Manta baixa este arquivo a cada verificação de atualizações e o aplica na próxima vez que abre. Serve para incluir emuladores novos e corrigir de onde um emulador embutido é baixado, tudo sem lançar uma versão nova do app. |
| [`emuladores.json`](emuladores.json) | Espelho de referência do catálogo **embutido** no app (`hub/src/emuland/model.cpp`). O app não lê este arquivo. |

## Como funciona

1. Ao procurar atualizações, o Manta baixa
   `https://raw.githubusercontent.com/<repo do Manta>/main/catalog/online.json`. Essa consulta não
   conta no limite de 60 por hora da API do GitHub.
2. Se o documento for válido, ele é guardado em `/data/emuland/catalog.json`. Se houver emulador
   novo, um aviso diz "1 emulador novo no catálogo: reabra o Manta para ver".
3. Ao abrir, o Manta junta essa cópia ao catálogo embutido, antes de montar as telas.

**Validação rígida.** O arquivo diz de onde o Manta baixa apps e qual pasta "Apagar tudo" remove.
Por isso, **qualquer** regra quebrada invalida o documento inteiro, e nada é aplicado:

- `id`, `usb` e `key` das pastas: minúsculas, dígitos e `-`.
- `title_id`: quatro letras maiúsculas e cinco dígitos, sem repetir nenhum já usado.
- `repo`: `dono/nome`.
- `data`: uma pasta própria logo abaixo de `/data/`, como `/data/meuemu`. Nunca `/data`,
  `homebrew`, `emuland` nem um caminho com `..`.
- `path` de cada pasta: relativo à pasta de dados, sem `..`.
- Textos sem caracteres de controle. No máximo 32 emuladores e 12 pastas por emulador.

Um emulador com `"min_version": "1.4.0"` só aparece para quem tem essa versão do Manta ou mais nova.

## Formato

```json
{
  "schema": 1,
  "emulators": [
    {
      "id": "meuemu",
      "name": "MeuEmu",
      "short_name": "GBA",
      "system": {"pt": "Game Boy Advance", "en": "Game Boy Advance"},
      "maker": "nintendo",
      "author": "fulano",
      "license": "GPL-3.0",
      "repo": "fulano/MeuEmu-PS5",
      "title_id": "PPSA97777",
      "asset_prefix": "MeuEmu-",
      "prerelease": false,
      "data": "/data/meuemu",
      "usb": "gba",
      "rom_option": "--rom",
      "needs": {"pt": "BIOS", "en": "BIOS"},
      "description": {"pt": "Emulador de GBA nativo.", "en": "Native GBA emulator."},
      "highlights": [{"pt": "60 fps", "en": "60 fps"}],
      "requirements": ["etaHEN + kstuff", "ShadowMountPlus"],
      "folders": [
        {"key": "jogos", "name": {"pt": "Jogos", "en": "Games"}, "path": "games",
         "note": {"pt": ".gba", "en": ".gba"}, "need": "none", "games": true, "extensions": "gba,zip"},
        {"key": "bios", "name": {"pt": "BIOS", "en": "BIOS"}, "path": "bios",
         "note": {"pt": "gba_bios.bin", "en": "gba_bios.bin"}, "need": "required", "extensions": "bin"}
      ]
    }
  ],
  "changes": [
    {"id": "psxs5", "repo": "SynoPiia/PSXS5", "asset_prefix": "PPSA97510"}
  ]
}
```

- `maker`: `sony`, `nintendo`, `microsoft` ou `multi` (a prateleira do Início).
- `asset_prefix`: começo do nome do `.zip` na release. Vazio quando o projeto só publica
  código-fonte.
- `rom_option`: opção que abre um jogo direto, se o emulador aceitar.
- `need` de uma pasta: `none`, `optional` ou `required`.
- `changes`: para um emulador **embutido** (pelo `id`), troca só `repo`, `asset_prefix`,
  `rom_option` ou `prerelease`.

Emuladores do catálogo online aparecem sem ícone e sem arte de fundo, porque a arte vem embutida no
app. Na próxima versão do Manta, ele pode passar para o catálogo embutido com a arte.
