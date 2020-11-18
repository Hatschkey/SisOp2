# Sistemas Operacionais 2 
## TRABALHO PRÁTICO PARTE 1: THREADS, SINCRONIZAÇÃO E COMUNICAÇÃO

Como compilar:

  - No diretório raiz, rodar `make`

Como utilizar:

- Rodar `make run_server` para inicializar o servidor com N = 50
- Rodar `make run_client` para inicializar a sessão de um usuário cujo nome de usuário é user, tentando se conectar a um grupo de nome group
- Ou então, entrar no diretório `{root}/bin` e rodar os seguintes comandos:
`./server N` onde o parâmetro N indica as últimas N mensagens que se deseja trazer no histórico de mensagens do grupo 
`./client username groupname ip port` onde username é o username do usuário que se deseja conectar, groupname é o nome do grupo que se deseja entrar, ip(usa-se 127.0.0.1 para o ip local) é o ip ao qual vamos nos conectar e port é a porta que será usada para estabelecer a conexão

## TRABALHO PRÁTICO PARTE 2: REPLICAÇÃO PASSIVA E ELEIÇÃO DE LÍDER

### Compile:
- Rodar `make` no diretório `root`

### Run:
- Replicas / Servidor: 
  - No diretório `{root}/bin` rodar:
    - `./replica N replica-port replica-ID leader-ip leader-port leader-ID`
    - Quando `replica-ID` é igual a `leader-ID`, esta replica se comporta como se fosse o líder
    - Para as demais réplicas, utilizar `leader-ID` e `leader-port` iguais àquelas utilizadas no processo líder.
  - OBS.1: Novamente, utilizar `127.0.0.1` para o IP, dado que os processos rodam na mesma máquina
  - OBS.2: Rodar o líder primeiro, e depois as demais replicas para que a conexão possa ser estabelecida
  - Exemplo:
  ```
  # 1º Inicia o processo líder (id e porta iguais)
  ~$ ./replica 5 6789 0 127.0.0.1 6789 0 Réplica ID 0 (Líder)

  # 2º Inicia as demais replicas (id e porta diferentes)
  ~$ ./replica 5 6788 1 127.0.0.1 6789 0 # Replica ID 1
  ~$ ./replica 5 6787 2 127.0.0.1 6789 0 # Replica ID 2
  ...
  ```
  - Alternativamente, rodar `make run_replicas` para incializar 3 réplicas em terminais `xterm`
- Cliente / Front-End:
  - TODO
