#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <unistd.h> 
#include <arpa/inet.h> 
#include <math.h>
#include <omp.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>

#define PORTA 8080 
#define BUFFER_SIZE 1024 
#define MAX_CACHE 1000

// Estrutura para cache
typedef struct {
    long long numero;
    int resultado;
} CacheEntry;

// Cache global e mutex
CacheEntry cache[MAX_CACHE];
int cache_count = 0;
pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;
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

// Funcao para verificar cache
int verificar_cache(long long num, int *resultado) {
    pthread_mutex_lock(&cache_mutex);
    for (int i = 0; i < cache_count; i++) {
        if (cache[i].numero == num) {
            *resultado = cache[i].resultado;
            pthread_mutex_unlock(&cache_mutex);
            return 1;
        }
    }
    pthread_mutex_unlock(&cache_mutex);
    return 0;
}

// Funcao para adicionar ao cache
void adicionar_cache(long long num, int resultado) {
    pthread_mutex_lock(&cache_mutex);
    if (cache_count < MAX_CACHE) {
        cache[cache_count].numero = num;
        cache[cache_count].resultado = resultado;
        cache_count++;
    }
    pthread_mutex_unlock(&cache_mutex);
}

// Funcao otimizada para testar primalidade
int is_prime(long long num) {
    if (num <= 1) return 0;
    if (num == 2) return 1;
    if (num % 2 == 0) return 0;
    if (num == 3) return 1;
    if (num % 3 == 0) return 0;

    long long limit = (long long)sqrt(num);
    int result = 1;
    
    #pragma omp parallel for reduction(&&:result)
    for (long long i = 5; i <= limit; i += 6) {
        if (num % i == 0 || num % (i + 2) == 0) {
            result = 0;
        }
    }
    return result;
}

// Estrutura para passar dados à thread
typedef struct {
    int socket_fd;
    int id_cliente;
} ClienteArgs;

// Funcao para processar cada cliente em uma thread
void* processar_cliente(void *arg) {
    ClienteArgs *args = (ClienteArgs *)arg;
    int novo_socket = args->socket_fd;
    int id_cliente = args->id_cliente;
    
    char buffer[BUFFER_SIZE] = {0};
    char mensagem_resposta[BUFFER_SIZE] = {0};
    struct timeval inicio, fim;
    
    log_mensagem("[Cliente %d] Conectado", id_cliente);
    
    while(1) {
        memset(buffer, 0, sizeof(buffer));
        memset(mensagem_resposta, 0, sizeof(mensagem_resposta));

        // Lendo o numero enviado pelo cliente
        ssize_t status = read(novo_socket, buffer, BUFFER_SIZE);

        if (status <= 0) {
            log_mensagem("[Cliente %d] Desconectado", id_cliente);
            break;
        }

        long long numero = atoll(buffer);
        int resultado;
        
        gettimeofday(&inicio, NULL);
        
        // Verificando cache
        if (verificar_cache(numero, &resultado)) {
            log_mensagem("[Cliente %d] Numero %lld encontrado em cache", id_cliente, numero);
            if (resultado) {
                strcpy(mensagem_resposta, "Primo");
            } else {
                strcpy(mensagem_resposta, "Nao Primo");
            }
        } else {
            log_mensagem("[Cliente %d] Testando primalidade de: %lld", id_cliente, numero);
            
            // Testando primalidade
            resultado = is_prime(numero);
            
            if (resultado) {
                strcpy(mensagem_resposta, "Primo");
            } else {
                strcpy(mensagem_resposta, "Nao Primo");
            }
            
            // Adicionando ao cache
            adicionar_cache(numero, resultado);
        }
        
        gettimeofday(&fim, NULL);
        double tempo_ms = (fim.tv_sec - inicio.tv_sec) * 1000.0 + 
                         (fim.tv_usec - inicio.tv_usec) / 1000.0;
        
        log_mensagem("[Cliente %d] Resultado: %lld %s (%.2f ms)", id_cliente, numero, mensagem_resposta, tempo_ms);

        // Enviando resposta ao cliente
        send(novo_socket, mensagem_resposta, strlen(mensagem_resposta), 0);
    }

    close(novo_socket);
    free(args);
    pthread_exit(NULL);
}

int main() { 
    int servidor_fd, novo_socket; 
    struct sockaddr_in endereco; 
    int tamanho_endereco = sizeof(endereco); 
    pthread_t thread_cliente;
    int id_cliente = 0;

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

    // 4. Esperando por conexoes (Listen) - Limite de 10 conexoes na fila 
    if (listen(servidor_fd, 10) < 0) { 
        perror("Falha no listen"); 
        close(servidor_fd); 
        exit(EXIT_FAILURE); 
    } 

    log_mensagem("Servidor aguardando conexoes na porta %d", PORTA); 

    // 5. Loop aceitando conexoes
    while(1) {
        if ((novo_socket = accept(servidor_fd, (struct sockaddr *)&endereco, (socklen_t*)&tamanho_endereco)) < 0) { 
            perror("Falha no accept"); 
            continue;
        }

        id_cliente++;
        
        // Criando thread para processar cliente
        ClienteArgs *args = malloc(sizeof(ClienteArgs));
        args->socket_fd = novo_socket;
        args->id_cliente = id_cliente;
        
        if (pthread_create(&thread_cliente, NULL, processar_cliente, args) != 0) {
            perror("Erro ao criar thread");
            free(args);
            close(novo_socket);
            continue;
        }
        
        pthread_detach(thread_cliente);
    }

    close(servidor_fd); 
    return 0; 
}