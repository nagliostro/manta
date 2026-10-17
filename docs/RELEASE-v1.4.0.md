## Manta v1.4.0

O Manta reúne todos os emuladores do seu PS5 em um só lugar. Esta versão traz o **instalador de um
passo** (sem FTP), a **autoatualização** e o **catálogo online**.

### Como instalar

**Jeito mais fácil (sem FTP):** baixe **`Manta-v1.4.0-installer.elf`** e envie para o ELF loader
do PS5 (porta 9021), como qualquer payload. O PS5 avisa quando terminar, e o Manta aparece na tela
inicial.

**Por FTP:** baixe **`Manta-v1.4.0-PPSA98265.zip`**, extraia e copie a pasta `PPSA98265` para
`/data/homebrew/` com permissão `777`.

Requisitos: PS5 com HEN (acesso a `/data`), kstuff e ShadowMountPlus. O passo a passo completo
está no [README](https://github.com/nagliostro/manta#instalação).

### Novidades
- **Instalador `.elf` de um passo:** envie para a porta 9021 e pronto. Ele leva o app dentro,
  então funciona sem internet.
- **O Manta se atualiza sozinho:** a partir desta versão, as próximas aparecem em Atualizações.
  Ele baixa e confere a versão nova, e a troca é feita assim que você fecha o app.
- **Catálogo online:** emuladores novos chegam sem precisar de versão nova do app.

**Vindo da v1.3.0?** Instale esta por cima com o `.elf` ou por FTP. Daqui em diante, o próprio
Manta cuida das atualizações.

Lista completa: [CHANGELOG](https://github.com/nagliostro/manta/blob/main/CHANGELOG.md).

### Verificação
O `SHA256SUMS.txt` traz o hash de cada arquivo.

**Testado em:** firmware 13.60, LegacyJB + kstuff + ShadowMountPlus.
