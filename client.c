#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>

#define PORTA 8080 
#define BUFFER_SIZE 1024 
#define NUM_THREADS 4

// Estrutura para passar dados à thread
typedef struct {
    char ip[16];
    int porta;
    char numero[BUFFER_SIZE];
    int id_thread;
} ThreadArgs;

pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Funcao para registrar logs
void log_mensagem(const char *formato, ...) {
    va_list args;
    va_start(args, formato);
    
    pthread_mutex_lock(&log_mutex);
    
    time_t agora = time(NULL);
    struct tm *tempo_local = localtime(&agora);
    char timestamp[30];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tempo_local);
    
    printf("[%s] ", timestamp);
    vprintf(formato, args);
    printf("\n");
    fflush(stdout);
    
    pthread_mutex_unlock(&log_mutex);
    va_end(args);
}

#include <stdarg.h>

// Funcao para processar cada numero em uma thread
void* processar_numero(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    struct timeval inicio, fim;
    
    // Criando socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_mensagem("[Thread %d] Erro ao criar socket", args->id_thread);
        free(args);
        pthread_exit(NULL);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(args->porta);
    
    if (inet_pton(AF_INET, args->ip, &serv_addr.sin_addr) <= 0) {
        log_mensagem("[Thread %d] Endereco invalido", args->id_thread);
        close(sock);
        free(args);
        pthread_exit(NULL);
    }
    
    // Conectando ao servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        log_mensagem("[Thread %d] Falha ao conectar ao servidor", args->id_thread);
        close(sock);
        free(args);
        pthread_exit(NULL);
    }
    
    log_mensagem("[Thread %d] Conectado ao servidor", args->id_thread);
    
    gettimeofday(&inicio, NULL);
    
    log_mensagem("[Thread %d] Enviando numero: %s", args->id_thread, args->numero);
    
    // Enviando numero
    send(sock, args->numero, strlen(args->numero), 0);
    
    // Recebendo resposta
    memset(buffer, 0, sizeof(buffer));
    read(sock, buffer, BUFFER_SIZE);
    
    gettimeofday(&fim, NULL);
    double tempo_ms = (fim.tv_sec - inicio.tv_sec) * 1000.0 + 
                     (fim.tv_usec - inicio.tv_usec) / 1000.0;
    
    log_mensagem("[Thread %d] Resposta do servidor: %s (%.2f ms)", args->id_thread, buffer, tempo_ms);
    
    close(sock);
    free(args);
    pthread_exit(NULL);
}

int main() { 
    char numero[BUFFER_SIZE] = {0};
    char portInput[5];
    int port;
    char ip[16];
    FILE *arquivo;
    pthread_t threads[NUM_THREADS];
    int thread_index = 0;

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

    port = atoi(portInput);
    log_mensagem("Conectando a %s:%d com %d threads", ip, port, NUM_THREADS);

    // 1. Abrindo o arquivo de numeros
    arquivo = fopen("numeros.txt", "r");
    if (arquivo == NULL) {
        log_mensagem("Erro ao abrir o arquivo numeros.txt");
        return -1;
    }

    log_mensagem("=== Iniciando teste de primalidade com multiplas threads ===");

    // 2. Lendo numeros do arquivo e criando threads
    while (fgets(numero, sizeof(numero), arquivo) != NULL) {
        // Remove a quebra de linha
        numero[strcspn(numero, "\n")] = '\0';

        // Ignora linhas vazias
        if (strlen(numero) == 0) continue;

        // Esperando thread anterior se necessario
        if (thread_index == NUM_THREADS) {
            pthread_join(threads[thread_index - 1], NULL);
            thread_index--;
        }
        
        // Criando thread para processar numero
        ThreadArgs *args = malloc(sizeof(ThreadArgs));
        strcpy(args->ip, ip);
        args->porta = port;
        strcpy(args->numero, numero);
        args->id_thread = thread_index + 1;
        
        if (pthread_create(&threads[thread_index], NULL, processar_numero, args) != 0) {
            log_mensagem("Erro ao criar thread");
            free(args);
            continue;
        }
        
        thread_index++;
    }

    // 3. Esperando todas as threads terminarem
    for (int i = 0; i < thread_index; i++) {
        pthread_join(threads[i], NULL);
    }

    // 4. Fechando arquivo
    fclose(arquivo);

    log_mensagem("=== Teste finalizado ===");
    return 0;
}