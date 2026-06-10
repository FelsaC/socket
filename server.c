#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <math.h>
#include <omp.h>

#define PORTA 8080 
#define BUFFER_SIZE 1024 

// Funcao para testar primalidade usando OpenMP
int is_prime(long long num) {
    if (num <= 1) return 0;
    if (num == 2) return 1;
    if (num % 2 == 0) return 0;

    long long limit = (long long)sqrt(num);
    int result = 1;

   #pragma omp parallel for reduction(&&:result)
    for (long long i = 3; i <= limit; i += 2) {
        if (result && num % i == 0) {
            result = 0;
        }
    }

    return result;
}

int main() { 
    int servidor_fd, novo_socket; 
    struct sockaddr_in endereco; 
    int tamanho_endereco = sizeof(endereco); 
    char buffer[BUFFER_SIZE] = {0}; 
    char mensagem_resposta[BUFFER_SIZE] = {0};

    // 1. Criando o descritor do socket (IPv4, TCP) 
    if ((servidor_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { 
        perror("Falha ao criar o socket"); 
        exit(EXIT_FAILURE); 
    } 

    // 2. Configurando a estrutura do endereço 
    endereco.sin_family = AF_INET; 
    endereco.sin_addr.s_addr = INADDR_ANY;
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

    printf("Cliente conectado!\n\n");

    // 6. Loop para receber e processar numeros
    while(1) {
        memset(buffer, 0, sizeof(buffer));
        memset(mensagem_resposta, 0, sizeof(mensagem_resposta));

        // Lendo o numero enviado pelo cliente
        ssize_t status = read(novo_socket, buffer, BUFFER_SIZE);

        if (status <= 0) {
            printf("Cliente desconectado.\n");
            break;
        }

        long long numero = atoll(buffer);

        printf("Testando primalidade de: %lld\n", numero);

        // Testando primalidade com OpenMP
        if (is_prime(numero)) {
            strcpy(mensagem_resposta, "Primo");
        } else {
            strcpy(mensagem_resposta, "Nao Primo");
        }

        printf("Resultado: %s\n\n", mensagem_resposta);

        // Enviando resposta ao cliente
        send(novo_socket, mensagem_resposta, strlen(mensagem_resposta), 0);
    }

    // 7. Fechando os sockets
    close(novo_socket); 
    close(servidor_fd); 

    return 0; 
}