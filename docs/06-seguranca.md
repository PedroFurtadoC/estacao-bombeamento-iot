# 06 - Segurança

O que o protótipo faz hoje e o que faltaria para isso rodar de verdade numa
estação de bombeamento. Esta é a seção de segurança do documento técnico.

## O que está implementado

Senha nenhuma entra no repositório. O `firmware/src/secrets.h`, com o Wi-Fi e o
endereço do broker, e o `infra/.env`, com as senhas do banco e do Grafana,
estão no `.gitignore`; o que se versiona são os arquivos `.example`, com
valores de mentira para quem for clonar preencher.

As ESP32 conectam no hotspot do notebook, que é uma rede só nossa. Isso também
foi recomendação do professor, mas ajuda na segurança: nada disso fica exposto
na rede da universidade.

Broker, banco e Grafana sobem em contêiner com as portas publicadas só para o
host. Não há nada escutando na internet.

O InfluxDB exige token e o Grafana exige usuário e senha, ambos definidos no
`.env`. O token que o Grafana usa é de leitura no bucket `iot`; o de escrita
fica só com o serviço de ingestão, que é quem precisa gravar.

A ingestão valida tudo que chega: JSON que não é JSON, máquina fora da lista,
campo obrigatório ausente ou vindo como texto, nada disso vira ponto no
banco, só uma linha de log. Um dispositivo com defeito, ou alguém publicando
lixo no tópico, não contamina a base.

Por último, o LWT do MQTT: se uma placa cai, o broker publica `offline` no
tópico de status dela. Disponibilidade também é segurança: saber que um nó
está mudo é diferente de achar que está tudo bem porque não chegou alerta.

## O que não está, e sabemos

O MQTT roda sem TLS e o broker aceita conexão anônima. Dentro do hotspot, numa
bancada, isso é aceitável; em produção não seria, porque qualquer um na mesma
rede consegue ler a telemetria e, pior, publicar telemetria falsa.

O InfluxDB não tem criptografia em repouso, e o Grafana tem um único usuário
administrador.

Essas três coisas não foram esquecimento: são o limite consciente de um
protótipo de bancada, e é por isso que estão listadas aqui.

## Como seria em produção

**Transporte.** MQTT sobre TLS 1.2 ou superior, na 8883, com certificado por
dispositivo (mTLS). HTTPS no Grafana e no InfluxDB.

**Identidade.** Um usuário ou certificado por ESP32, com ACL por tópico: a M01
só pode publicar em `fabrica/maquinas/M01/#`. Assim uma placa comprometida não
consegue falsear as outras.

**Segredos.** Cofre (Vault, SOPS) no lugar de arquivo, com rotação periódica
dos tokens.

**Rede.** Segmentação, com a rede de automação separada da rede administrativa,
firewall entre elas e nenhuma porta de banco exposta.

**Acesso.** SSO e papéis no Grafana: operador vê, engenheiro edita.

**Auditoria.** Log de acesso e de alteração retido por anos, que é o que
permite reconstruir o que aconteceu depois de um incidente.

**Atualização.** OTA assinada para o firmware. Placa em campo que não pode ser
atualizada com segurança vira passivo no dia que aparecer uma falha.

## Que dados merecem proteção

Credenciais e tokens, por motivo óbvio. A telemetria, porque estação de água é
infraestrutura crítica e a série revela o padrão de abastecimento, e porque
quem escreve no tópico pode esconder uma falha real. Os eventos de falha, que
têm peso contratual entre quem opera e quem mantém. E os dados de acesso dos
usuários do dashboard.
