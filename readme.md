# Sistemas Operacionais 2 
## TRABALHO PRÁTICO PARTE 1: THREADS, SINCRONIZAÇÃO E COMUNICAÇÃO

Como compilar:

  - No diretório raiz, rodar `make`

Como utilizar:

- Rodar `make run_server` para inicializar o servidor e um grupo
- Rodar `make run_client` para inicializar a sessão de um usuário cujo nome de usuário é user
- Ou então, entrar no diretório `{root}/bin` e rodar os seguintes comandos:
`./server N` onde o parâmetro N indica as últimas N mensagens que se deseja trazer no histórico de mensagens do grupo 
`./client username groupname ip port` onde username é o username do usuário que se deseja conectar, groupname é o nome do grupo que se deseja entrar, ip(usa-se 127.0.0.1 para o ip local) é o ip ao qual vamos nos conectar e port é a porta que será usada para estabelecer a conexão
