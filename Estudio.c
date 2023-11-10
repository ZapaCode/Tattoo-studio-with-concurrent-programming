// Bernardo José Zaparoli Matrícula: 189797
// Andre Pelin Matrícula: 173702

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define MAX_CLIENTES 100
#define MIN_PUF_TATTOO 0
#define MAX_PUF_TATTOO 10
#define MIN_PUF_PIERCING 0
#define MAX_PUF_PIERCING 10

#define PROB_TATTOO 0.99
#define MIN_TEMPO_SERVICO 100
#define MAX_TEMPO_SERVICO 500

pthread_t recepcao;
pthread_t tattoo_artist;
pthread_t piercing_artist;

pthread_mutex_t mutex;
pthread_cond_t cond;

int num_clientes_servidos = 0;
int num_tattoo_clientes_servidos = 0;
int num_piercing_clientes_servidos = 0;
int num_clientes_esquerda = 0;
int num_clientes_esperando = 0;

int num_tattoo_pufs = 0;
int num_piercing_pufs = 0;

bool studio_aberto = true;
int max_clientes;

void log_client(int id, const char *action) {
    printf("Cliente #%d %s\n", id, action);
}

void *tattoo_thread(void *arg);
void *piercing_thread(void *arg);

void *recepcao_thread(void *arg) {
    while (num_clientes_servidos < max_clientes) {
        usleep(100 * 60 * 1000); // A cada 100 minutos
        int num_novos_clientes = (rand() % 10) + 1; // Entre 1 e 10 clientes

        for (int i = 0; i < num_novos_clientes; i++) {
            pthread_t client;
            pthread_create(&client, NULL, (rand() / (double)RAND_MAX) <= PROB_TATTOO ? tattoo_thread : piercing_thread, NULL);
        }
    }

    pthread_mutex_lock(&mutex);
    studio_aberto = false;
    pthread_mutex_unlock(&mutex);
    pthread_cond_broadcast(&cond);
    printf("Estúdio fechado. Não estão mais permitidos novos clientes.\n");
    pthread_join(tattoo_artist, NULL);
    pthread_join(piercing_artist, NULL);

    return NULL;
}

void *tattoo_thread(void *arg) {
    int id = rand() % 1000 + 1; // Número aleatório entre 1 e 1000

    pthread_mutex_lock(&mutex);
    log_client(id, "chegou para fazer uma tatuagem.");

    if (!studio_aberto) {
        num_clientes_esquerda++;
        log_client(id, "chegou, mas o estúdio está fechado, foi embora.");
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    if (num_tattoo_pufs < max_clientes) {
        log_client(id, "chegou e foi sentar num puf de espera.");
        num_tattoo_pufs++;
    } else {
        num_clientes_esquerda++;
        log_client(id, "chegou e foi embora pois não foi atendido e não podia sentar (não há pufs disponíveis).");
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    pthread_mutex_unlock(&mutex);

    int tempo_servico = (rand() % (MAX_TEMPO_SERVICO - MIN_TEMPO_SERVICO)) + MIN_TEMPO_SERVICO;
    usleep(tempo_servico * 1000); // Tempo de atendimento em milissegundos

    pthread_mutex_lock(&mutex);
    num_tattoo_pufs--;
    log_client(id, "terminou a tatuagem.");
    num_clientes_servidos++;
    num_tattoo_clientes_servidos++;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void *piercing_thread(void *arg) {
    int id = rand() % 1000 + 1; // Número aleatório entre 1 e 1000

    pthread_mutex_lock(&mutex);
    log_client(id, "chegou para fazer um body piercing.");

    if (!studio_aberto) {
        num_clientes_esquerda++;
        log_client(id, "chegou, mas o estúdio está fechado, foi embora.");
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    if (num_piercing_pufs < max_clientes) {
        log_client(id, "chegou e foi sentar num puf de espera.");
        num_piercing_pufs++;
    } else {
        num_clientes_esquerda++;
        log_client(id, "chegou e foi embora pois não foi atendido e não podia sentar (não há pufs disponíveis).");
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    pthread_mutex_unlock(&mutex);

    int tempo_servico = (rand() % (MAX_TEMPO_SERVICO - MIN_TEMPO_SERVICO)) + MIN_TEMPO_SERVICO;
    usleep(tempo_servico * 1000); // Tempo de atendimento em milissegundos

    pthread_mutex_lock(&mutex);
    num_piercing_pufs--;
    log_client(id, "terminou o body piercing.");
    num_clientes_servidos++;
    num_piercing_clientes_servidos++;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 7) {
        printf("Uso: %s CLI PT PBP PAT MINATEN MAXATEN\n", argv[0]);
        return 1;
    }

    max_clientes = atoi(argv[1]);
    num_tattoo_pufs = atoi(argv[2]);
    num_piercing_pufs = atoi(argv[3]);

    if (max_clientes < 10 || max_clientes > 100 || num_tattoo_pufs < 0 || num_tattoo_pufs > 10 || num_piercing_pufs < 0 || num_piercing_pufs > 10) {
        printf("Valores inválidos para os argumentos.\n");
        return 1;
    }

    double pat = atof(argv[4]);
    
    if (pat <= 0 || pat >= 0.99) {
        printf("Valor inválido para PAT.\n");
        return 1;
    }

    int min_aten = atoi(argv[5]);
    int max_aten = atoi(argv[6]);

    if (min_aten <= 0 || max_aten < min_aten || max_aten > 500) {
        printf("Valores inválidos para MINATEN e MAXATEN.\n");
        return 1;
    }

    srand(time(NULL));

    if (pthread_mutex_init(&mutex, NULL) != 0) {
        printf("Erro ao inicializar o mutex.\n");
        return 1;
    }

    if (pthread_cond_init(&cond, NULL) != 0) {
        printf("Erro ao inicializar a variável de condição.\n");
        return 1;
    }

    pthread_create(&recepcao, NULL, recepcao_thread, NULL);
    pthread_create(&tattoo_artist, NULL, tattoo_thread, NULL);
    pthread_create(&piercing_artist, NULL, piercing_thread, NULL);

    pthread_join(recepcao, NULL);

    while (num_clientes_esperando > 0) {
        pthread_mutex_lock(&mutex);
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }

    printf("Resumo do dia:\n");
    printf("Total de clientes atendidos: %d\n", num_clientes_servidos);
    printf("Clientes atendidos por tatuadores: %d\n", num_tattoo_clientes_servidos);
    printf("Clientes atendidos por body piercers: %d\n", num_piercing_clientes_servidos);
    printf("Clientes que chegaram e foram embora sem serem atendidos: %d\n", num_clientes_esquerda);
    printf("Clientes atendidos sem esperar nos pufs: %d\n", num_clientes_esperando);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}