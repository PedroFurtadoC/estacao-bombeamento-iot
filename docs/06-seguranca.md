# 06 - Segurança

Seção "segurança" do documento técnico: o que o protótipo implementa e o que
uma implantação real exigiria.

## Implementado no protótipo

| Medida | Onde |
|---|---|
| Segredos fora do versionamento | `firmware/src/secrets.h` e `infra/.env` estão no `.gitignore`; o repositório só contém `*.example` |
| Rede isolada | ESP32 conectam ao **hotspot do notebook**, rede própria, sem exposição à rede da universidade |
| Serviços apenas locais | Broker, banco e Grafana escutam somente em `localhost` (portas publicadas para o host, sem exposição externa) |
| Autenticação no banco e no dashboard | InfluxDB com token; Grafana com usuário/senha definidos no `.env` |
| Menor privilégio no Grafana | Token do InfluxDB usado pelo Grafana é de leitura no bucket `iot` (o token de escrita fica só na ingestão) |
| Validação de entrada | O serviço de ingestão valida o JSON (campos, tipos, faixas) e descarta payloads malformados |
| Última vontade (LWT) | Tópico `status` detecta dispositivo offline; disponibilidade também é segurança |

## Limitações conscientes do protótipo (escala didática)

- MQTT **sem TLS** e broker com acesso anônimo dentro do hotspot: aceitável na
  bancada, inaceitável em produção;
- Sem criptografia em repouso no InfluxDB;
- Um único usuário administrador no Grafana.

## Como seria em produção

1. **Transporte:** MQTT sobre TLS 1.2+ (porta 8883) com certificado por
   dispositivo (mTLS); HTTPS no Grafana/InfluxDB;
2. **Identidade:** um usuário/certificado por ESP32 no broker, com ACL por
   tópico (`M01` só publica em `fabrica/maquinas/M01/#`);
3. **Segredos:** cofre (Vault/SOPS) e rotação periódica de tokens;
4. **Rede:** segmentação (VLAN OT separada da TI), firewall e sem portas de
   banco expostas;
5. **Acesso:** SSO + papéis no Grafana (operador vê, engenheiro edita);
6. **Auditoria:** logs de acesso e de alteração retidos por anos;
7. **Atualização:** OTA assinada para o firmware das ESP32.

## Dados a proteger (resumo da pergunta 9 do Desafio 4)

Credenciais e tokens; telemetria (estação de bombeamento é infraestrutura
crítica; o dado revela o padrão operacional do abastecimento); eventos de
falha (sensíveis contratualmente); dados de acesso dos usuários do dashboard.
