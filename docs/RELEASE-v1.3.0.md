## Manta v1.3.0

Primeira versão pública do Manta, o app que reúne todos os emuladores do seu PS5 em um só lugar:
instale, atualize, remova e jogue, com uma biblioteca única de jogos.

### Como instalar

1. Baixe **`Manta-v1.3.0-PPSA98265.zip`** abaixo e extraia. Você terá a pasta `PPSA98265`.
2. Com o PS5 desbloqueado (HEN com acesso a `/data`, kstuff e ShadowMountPlus) e o FTP ligado
   (porta 2121), copie **a pasta inteira** para `/data/homebrew/`, com permissão `777`.
   Ou: `python scripts/deploy-ps5.py <IP-do-PS5> --folder PPSA98265`.
3. Espere o Manta aparecer na tela inicial e abra.

O passo a passo completo está no [README](../README.md#instalação).

### Novidades
- **"Abrir em qual emulador?"**: quando dois emuladores instalados rodam o mesmo jogo (PSXS5 e
  SwanStationPS5 no PS1), o Manta pergunta qual usar.
- Instalação, atualização e remoção de 8 emuladores, com SHA-256 conferido.
- Biblioteca de jogos com capas automáticas.
- Importar jogos e BIOS de um pendrive, sem saber caminho nenhum.

### Segurança e correções
- O Manta só apaga ou substitui a pasta de um app depois de confirmar que ela é daquele app.
- Imagens gigantes são recusadas antes de decodificar.
- Correções de estabilidade (subpastas com links, arquivo de idioma inválido).

Lista completa: [CHANGELOG](../CHANGELOG.md).

### Verificação
`SHA256SUMS.txt` traz o hash do ZIP.

**Testado em:** firmware 13.60, LegacyJB + kstuff + ShadowMountPlus.
