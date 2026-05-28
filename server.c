#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORTA 8080
#define BUFFER_SIZE 1024

int main() {
    int servidor_fd, novo_socket;
    struct sockaddr_in endereco;
    int tamanho_endereco = sizeof(endereco);
    char buffer[BUFFER_SIZE] = {0};
    char *mensagem_resposta = "Ola Cliente! Sua mensagem foi recebida com sucesso.";

    // 1. Criando o descritor do socket (IPv4, TCP)
    if ((servidor_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Falha ao criar o socket");
        exit(EXIT_FAILURE);
    }

    // 2. Configurando a estrutura do endereço
    endereco.sin_family = AF_INET;
    endereco.sin_addr.s_addr = INADDR_ANY; // Aceita conexoes de qualquer IP
    endereco.sin_port = htons(PORTA);

    // 3. Vinculando o socket a porta 8080 (Bind)
    if (bind(servidor_fd, (struct sockaddr *)&endereco, sizeof(endereco)) < 0) {
        perror("Falha no bind");
        close(servidor_fd);
        exit(EXIT_FAILURE);
    }

    // 4. Esperando por conexoes (Listen) - Limite de 3 conexoes na fila
    if (listen(servidor_fd, 3) < 0) {
        perror("Falha no listen");
        close(servidor_fd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor aguardando conexoes na porta %d...\n", PORTA);

    // 5. Aceitando a conexao do cliente (Accept)
    if ((novo_socket = accept(servidor_fd, (struct sockaddr *)&endereco, (socklen_t*)&tamanho_endereco)) < 0) {
        perror("Falha no accept");
        close(servidor_fd);
        exit(EXIT_FAILURE);
    }
  
    // 6. Lendo os dados enviados pelo cliente
    read(novo_socket, buffer, BUFFER_SIZE);
    printf("cliente: %s\n", buffer);

    // 7. Respondendo ao cliente
    send(novo_socket, mensagem_resposta, strlen(mensagem_resposta), 0);
    printf("Resposta enviada ao cliente.\n");

    // 8. Fechando os sockets
    close(novo_socket);
    close(servidor_fd);

    return 0;
}