#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#define MAX_CLIENTES 100
#define MAX_PUF_TATTOO 10
#define MAX_PUF_PIERCING 10
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
int num_clientes_atendidos_sem_espera = 0;
int num_tattoo_pufs = 0;
int num_piercing_pufs = 0;
bool studio_aberto = true;
int max_clientes;
int tattoo_artist_disponivel = 1;
int piercing_artist_disponivel = 1;

typedef struct {
    int id;
    const char *action;
    time_t arrival_time;
    time_t start_time;
    time_t end_time;
} Client;

typedef struct {
    time_t arrival_time;
    time_t wait_time;
    time_t end_time;
} TimeInfo;

void log_professional(const char *professional, int id, const char *action) {
    printf("%s #%d %s\n", professional, id, action);
}

void log_client(const Client *client, const TimeInfo *time_info) {
    printf("Cliente #%d %s %ld segundos\n", client->id, client->action, time_info->arrival_time);
}

void *tattoo_thread(void *arg);
void *piercing_thread(void *arg);

void *recepcao_thread(void *arg) {
    while (num_clientes_servidos < max_clientes) {
        usleep(100 * 60 * 1000);

        if (num_clientes_servidos >= max_clientes) {
            break;
        }

        int num_novos_clientes = (rand() % 10) + 1;

        for (int i = 0; i < num_novos_clientes; i++) {
            pthread_t client;

            if (pthread_create(&client, NULL, (rand() / (double)RAND_MAX) <= 0.5 ? tattoo_thread : piercing_thread, NULL) != 0) {
                fprintf(stderr, "Erro ao criar uma thread de cliente.\n");
                exit(EXIT_FAILURE);
            }
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
    int id = rand() % 1000 + 1;

    pthread_mutex_lock(&mutex);
    TimeInfo time_info = {.arrival_time = time(NULL), .wait_time = 0, .end_time = 0};
    Client client = {id, "Chegou para fazer uma tatuagem. Tempo de espera: ", time_info.arrival_time, 0, 0};
    log_client(&client, &time_info);

    if (!studio_aberto) {
        num_clientes_esquerda++;
        client.action = "Chegou, mas o estúdio está fechado, foi embora.";
        log_client(&client, &time_info);
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    // Se o tatuador ou o body piercer estiverem disponíveis, atende o cliente imediatamente
    if (tattoo_artist_disponivel || piercing_artist_disponivel) {
        if (tattoo_artist_disponivel) {
            tattoo_artist_disponivel = 0; // O tatuador não está mais disponível
        } else {
            piercing_artist_disponivel = 0; // O body piercer não está mais disponível
        }

        client.start_time = time(NULL);

        // Calculando o tempo de espera
        time_info.wait_time = client.start_time - time_info.arrival_time;
        client.action = "Iniciou o procedimento. Tempo de espera: ";
        log_client(&client, &time_info);

        pthread_mutex_unlock(&mutex);

        int tempo_servico = (rand() % (MAX_TEMPO_SERVICO - MIN_TEMPO_SERVICO)) + MIN_TEMPO_SERVICO;
        usleep(tempo_servico * 1000);

        pthread_mutex_lock(&mutex);
        if (tattoo_artist_disponivel) {
            log_professional("Tatuador", id, "Iniciou a tatuagem.");
            num_tattoo_clientes_servidos++;
            log_professional("Tatuador", id, "Finalizou a tatuagem.");
        } else {
            log_professional("Body Piercer", id, "Iniciou o body piercing.");
            num_piercing_clientes_servidos++;
            log_professional("Body Piercer", id, "Finalizou o body piercing.");
        }

        time_info.end_time = time(NULL);
        client.end_time = time_info.end_time;
        client.action = "Terminou o procedimento. Tempo de serviço total: ";
        log_client(&client, &time_info);
        num_clientes_servidos++;

        if (time_info.wait_time == 0) {
            num_clientes_atendidos_sem_espera++;
        }

        tattoo_artist_disponivel = 1; // O tatuador está disponível novamente
        piercing_artist_disponivel = 1; // O body piercer está disponível novamente
        pthread_mutex_unlock(&mutex);
    } else {
        // Se nenhum profissional estiver disponível, o cliente espera em um puff
        if (num_tattoo_pufs < MAX_PUF_TATTOO) {
            num_tattoo_pufs++;
            num_clientes_esperando++;
            pthread_mutex_unlock(&mutex);
            usleep(500 * 1000); // Tempo de espera simulado enquanto o cliente está no puff
            pthread_mutex_lock(&mutex);
            num_tattoo_pufs--; // Cliente deixou o puff e vai embora
            num_clientes_esperando--;
            client.action = "Chegou e sentou no puff enquanto espera. Tempo de espera nos pufs: ";
            log_client(&client, &time_info);
        } else {
            // Se não houver pufs disponíveis, o cliente vai embora
            num_clientes_esquerda++;
            client.action = "Chegou e foi embora pois não foi atendido e não podia sentar (não há pufs disponíveis).";
            log_client(&client, &time_info);
        }

        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

void *piercing_thread(void *arg) {
    int id = rand() % 1000 + 1;

    pthread_mutex_lock(&mutex);
    TimeInfo time_info = {.arrival_time = time(NULL), .wait_time = 0, .end_time = 0};
    Client client = {id, "Chegou para fazer um body piercing. Tempo de espera: ", time_info.arrival_time, 0, 0};
    log_client(&client, &time_info);

    if (!studio_aberto) {
        num_clientes_esquerda++;
        client.action = "Chegou, mas o estúdio está fechado, foi embora.";
        log_client(&client, &time_info);
        pthread_mutex_unlock(&mutex);
        return NULL;
    }

    // Se o body piercer estiver disponível, atende o cliente imediatamente
    if (piercing_artist_disponivel) {
        piercing_artist_disponivel = 0; // O body piercer não está mais disponível
        client.start_time = time(NULL);

        // Calculando o tempo de espera real
        time_info.wait_time = client.start_time - time_info.arrival_time;
        client.action = "Iniciou o body piercing. Tempo de espera: ";
        log_client(&client, &time_info);

        pthread_mutex_unlock(&mutex);

        int tempo_servico = (rand() % (MAX_TEMPO_SERVICO - MIN_TEMPO_SERVICO)) + MIN_TEMPO_SERVICO;
        usleep(tempo_servico * 1000);

        pthread_mutex_lock(&mutex);
        log_professional("Body Piercer", id, "Iniciou o body piercing.");
        time_info.end_time = time(NULL);
        client.end_time = time_info.end_time;
        client.action = "Terminou o body piercing. Tempo de serviço total: ";
        log_client(&client, &time_info);
        num_clientes_servidos++;
        num_piercing_clientes_servidos++;
        log_professional("Body Piercer", id, "Finalizou o body piercing.");
        piercing_artist_disponivel = 1; // O body piercer está disponível novamente
        pthread_mutex_unlock(&mutex);
    } else {
        // Se o body piercer não estiver disponível, o cliente espera em um puff
        if (num_piercing_pufs < MAX_PUF_PIERCING) {
            num_piercing_pufs++;
            num_clientes_esperando++;
            pthread_mutex_unlock(&mutex);
            usleep(500 * 1000); // Tempo de espera simulado enquanto o cliente está no puff
            pthread_mutex_lock(&mutex);
            num_piercing_pufs--; // Cliente deixou o puff e vai embora
            num_clientes_esperando--;
            client.action = "Chegou e sentou no puff enquanto espera. Tempo de espera nos pufs: ";
            log_client(&client, &time_info);
        } else {
            // Se não houver pufs disponíveis, o cliente vai embora
            num_clientes_esquerda++;
            client.action = "Chegou e foi embora pois não foi atendido e não podia sentar (não há pufs disponíveis).";
            log_client(&client, &time_info);
        }

        pthread_mutex_unlock(&mutex);
    }

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

    if (max_clientes < 10 || max_clientes > 100 || num_tattoo_pufs < 0 || num_tattoo_pufs > MAX_PUF_TATTOO || num_piercing_pufs < 0 || num_piercing_pufs > MAX_PUF_PIERCING) {
        printf("Valores inválidos para os argumentos.\n");
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

    pthread_join(recepcao, NULL);

    while (num_clientes_esperando > 0) {
        pthread_mutex_lock(&mutex);
        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mutex);
    }

    printf("\nResumo do dia:\n");
    printf("Total de clientes atendidos: %d\n", num_clientes_servidos);
    printf("Clientes atendidos por tatuadores: %d\n", num_tattoo_clientes_servidos);
    printf("Clientes atendidos por body piercers: %d\n", num_piercing_clientes_servidos);
    printf("Clientes atendidos sem esperar nos pufs: %d\n", num_clientes_atendidos_sem_espera);
    printf("Clientes que chegaram e foram embora sem serem atendidos: %d\n", num_clientes_esquerda);

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond);

    return 0;
}
