#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "input.h"
#include "interface.h"
#include "processo.h"



#ifndef COMUM
#define COMUM
#include "comum.h"
#endif


int lin, col;
tempo t;
frame frames[FRAMES];
processo processos[PROCESSOS];

pthread_t tid[PROCESSOS];

int ultimo_proc = 0; // processo que fez a ultima acao

int modo_texto = 0; // modo de texto simples ou modo dinamico


// variáveis de exclusão mútua
pthread_mutex_t print_mutex; // mutex para printfs que precisam estar em sequencia
pthread_mutex_t mp_mutex; // memória principal


// mutex e cond para bloquear as threads no modo de apertar espaco pra ver a próxima acao
pthread_mutex_t mutex;
pthread_cond_t cond;


// mutex e cond para bloquear para mostrar o antes e depois das tabelas, durante cada acao
/*
	ex:
	p00 solicita uma pv
	tp e memória do p00 são mostradas, antes da acao ocorrrer
	pausa para o usuário ver as tabelas antes da modificadao <----- aqui
	tabelas são alteradas
	tp e memória modificadas são mostradas
	
*/
pthread_mutex_t mutex2;
pthread_cond_t cond2;

int ok = 0;


// sisteminha de ticket para garantir uma ordem de fila no wait e no mutex
int ticket = 0; // ticker para ser pego
int prox_ticket = 0; // ticket chamado



void inicio() {
	int i, j;
	
	// relógio
	t = (tempo) {0, 0};

	// inicializando memória, com -1 indicando que o frame está livre
	for(i = 0; i < FRAMES; i++) frames[i] = (frame) {-1, -1};
	
	
	// inicializando processos, com -1 nas entradas da TP, indicando que não estão alocadas
	for(i = 0; i < PROCESSOS; i++) {
		for(j = 0; j < PAGINAS_VIRTUAIS; j++) {
			processos[i].tabela[j] = -1;
		}
		
		for(j = 0; j < WORKING_SET; j++) {
			processos[i].working_set[j] = -1;
		}
		
		processos[i].entrada = INF;
	}
	
	// inicialização das variáveis de exclusão mútua
	pthread_mutex_init(&print_mutex, NULL);
	pthread_mutex_init(&mp_mutex, NULL);
	
	pthread_mutex_init(&mutex, NULL);
	pthread_cond_init(&cond, NULL);
	
	pthread_mutex_init(&mutex2, NULL);
	pthread_cond_init(&cond2, NULL);
	
	if(!modo_texto) interface_inicio();
}


void fim() {


	if(!modo_texto) interface_fim();

	// liberação das variáveis de exclusão mútua
	pthread_mutex_destroy(&print_mutex);
	pthread_mutex_destroy(&mp_mutex);
	
	pthread_mutex_destroy(&mutex);
	pthread_cond_destroy(&cond);

	pthread_mutex_destroy(&mutex2);
	pthread_cond_destroy(&cond2);

	
	exit(0);
}



void uso(char *nome) {
	printf("Uso: %s MODO\n\n", nome);
	printf("MODO: \"basico\" ou \"completo\", indicando a forma da divisão das tarefas\n");

	printf("\nExemplo: %s completo\n", nome);
}


int main(int argc, char *argv[]) {
	
	
	

	// verificação dos argumentos
	
	if(argc < 2) {	// quantidade incorreta de argumentos
		uso(argv[0]);
		exit(0);
	}
	
	
	if(strcmp(argv[1], "basico") == 0)
		modo_texto = 1;
	
	else if(strcmp(argv[1], "completo") == 0)
		modo_texto = 0;
	
	else {
		uso(argv[0]);
		exit(0);
	}
	
	
	int i;
	
	inicio();
	
	pthread_t tid2;
	
	
	// thread para pegar a entrada (não tem no modo texto)
	if(!modo_texto) {
		if(pthread_create(&tid2, NULL, thread_input, NULL)) {
			printf("Falha ao criar thread.\n");
		    exit(1);
		}
	}
	
	
	if(modo_texto) {
		printf("*****\n");
		printf("Por favor, maximize sua janela.\n");
		printf("*****\n");
		sleep(2);
		ok = 1;
	}
	
	int segundos = 0;
	
	int meu_ticket;
	
	for(i = 0; i < PROCESSOS; i++) {
	
		meu_ticket = ticket++;
		pthread_mutex_lock(&mutex);
		
		// checagem do ok (esperando o input do espaco) não ocorre no modo básico, onde o ok é sempre 1
		while(!ok || prox_ticket < meu_ticket)
			pthread_cond_wait(&cond, &mutex);
		
		novo_processo();
		
		if(!modo_texto) {
			ok = 0;
		}
		
		prox_ticket++;
		
		if(!modo_texto) atualiza_hora2(segundos);
		segundos += TEMPO_NOVOPROC;
		
		
		pthread_mutex_unlock(&mutex);
		
		if(modo_texto) {
			pthread_cond_broadcast(&cond);
			sleep(TEMPO_NOVOPROC);
		}
		
		else
			// regra de 3 pegando o maior entre os 2 tempos e fazendo ele ser 500 ms, e diminuindo o outro proporcionalmente
			usleep(TEMPO_NOVOPROC > TEMPO_NOVAREQ ? 50000 : TEMPO_NOVOPROC*50000/TEMPO_NOVAREQ);
	}
	
	// esperando processos terminarem
	for(i = 0; i < PROCESSOS; i++) {
		pthread_join(tid[i], NULL);
	}
	
	fim();
	
	pthread_exit(NULL);
	
	return 0;
}
