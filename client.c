#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h> 

#define PORTA 8080 
#define BUFFER_SIZE 1024 

int main() { 
    int sock = 0; 
    struct sockaddr_in serv_addr; 
    char numero[BUFFER_SIZE] = {0}; 
    char buffer[BUFFER_SIZE] = {0}; 
    char portInput[5];
    int port;
    char ip[16];
    FILE *arquivo;

    // 0. Info de conexao
    printf("Insira ip para a conexao: ");
    if(fgets(ip, sizeof(ip), stdin)) {
        int c;
        while(!strchr(ip, '\n') && (c = getchar()) != '\n' && c != EOF);
        ip[strcspn(ip, "\n")] = '\0';
    }

    printf("Insira a porta: ");
    if(fgets(portInput, sizeof(portInput), stdin)) {
        int c;
        while(!strchr(portInput, '\n') && (c = getchar()) != '\n' && c != EOF);
        portInput[strcspn(portInput, "\n")] = '\0';
    }

    printf("Tentado conectar a %s:%s...\n", ip, portInput);
    port = atoi(portInput);

    // 1. Criando o socket do cliente 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
        printf("\n Erro na criacao do socket \n"); 
        return -1; 
    } 

    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(port); 

    // 2. Convertendo o endereco IP de texto para binario
    if (inet_pton(AF_INET, ip, &serv_addr.sin_addr) <= 0) { 
        printf("\nEndereco invalido ou nao suportado \n"); 
        return -1; 
    } 

    // 3. Conectando ao servidor 
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
        printf("\nConexao falhou! O servidor esta rodando? \n"); 
        return -1; 
    } 

    // 4. Abrindo o arquivo de numeros
    arquivo = fopen("numeros.txt", "r");
    if (arquivo == NULL) {
        printf("Erro ao abrir o arquivo numeros.txt\n");
        close(sock);
        return -1;
    }

    printf("\n=== Iniciando teste de primalidade ===\n\n");

    // 5. Lendo numeros do arquivo e enviando ao servidor
    while (fgets(numero, sizeof(numero), arquivo) != NULL) {
        // Remove a quebra de linha
        numero[strcspn(numero, "\n")] = '\0';

        // Ignora linhas vazias
        if (strlen(numero) == 0) continue;

        printf("Enviando numero: %s\n", numero);

        // Envia o numero para o servidor
        send(sock, numero, strlen(numero), 0);

        // Limpa o buffer e recebe a resposta
        memset(buffer, 0, sizeof(buffer));
        read(sock, buffer, BUFFER_SIZE);

        printf("Servidor respondeu: %s\n\n", buffer);
    }

    // 6. Fechando arquivo e socket
    fclose(arquivo);
    close(sock);

    printf("=== Teste finalizado ===\n");
    return 0;
}