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
    char mensagem[BUFFER_SIZE] = "Ola Servidor, sou o Cliente!"; 
    char buffer[BUFFER_SIZE] = {0}; 

    // 1. Criando o socket do cliente 
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) { 
      printf("\n Erro na criacao do socket \n"); 
      return -1; 
    } 

    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(PORTA); 

    // 2. Convertendo o endereco IP de texto para binario (localhost) 
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) { 
      printf("\nEndereco invalido ou nao suportado \n"); 
      return -1; 
    } 

 

    // 3. Conectando ao servidor 
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
      printf("\nConexao falhou! O servidor esta rodando? \n"); 
      return -1; 
    } 

    while(1) {
      memset(mensagem, 0, sizeof(mensagem));
      printf("Voce: ");
      fgets(mensagem, sizeof(mensagem), stdin);
      
      // 4. Enviando dados para o servidor 
      send(sock, mensagem, strlen(mensagem), 0); 
      // printf("Mensagem enviada ao servidor.\n"); 
  
      // 5. Lendo a resposta do servidor 
      memset(buffer, 0, sizeof(buffer));
      read(sock, buffer, BUFFER_SIZE); 
      printf("\nServidor: %s", buffer); 
      
      if(strncmp(buffer, "sair", 4) == 0) {
        printf("Fechando sockets...\n");
        send(sock, buffer, strlen(buffer), 0);
        break; 
      }
    }
    
    // 6. Fechando o socket 
    close(sock); 

    return 0; 

} 