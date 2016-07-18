#include "processo.h"

int proc_num = 0;

int ultimo_tempo = 0;
int num_entrada = 0;

int esperando = 0; // espera para o usuário ver as tabelas antes de serem modificadas

void *thread_processo(void *arg) {
	int proc = *(int *)arg; // numero do "processo"
	free(arg);
	srand(time(NULL));
	
	int segundos = proc * TEMPO_NOVOPROC;
	
	
	msg3("[%02d:%02d] P%02d: iniciei", segundos/60, segundos%60, proc);
	
	int meu_ticket;
	
	while(1) {
		
		meu_ticket = ticket++;
		pthread_mutex_lock(&mutex);
		
		// checagem do ok (esperando o input do espaco) não ocorre no modo básico, onde o ok é sempre 1
		while(!ok || prox_ticket < meu_ticket)
			pthread_cond_wait(&cond, &mutex);


		ultimo_tempo = segundos;
		if(!modo_texto) atualiza_hora2(segundos);
		
		solicita_pv(proc, (rand() % PAGINAS_VIRTUAIS));
		
		if(!modo_texto) {
			ok = 0;
		}
		
		prox_ticket++;
		
		segundos += TEMPO_NOVAREQ;
		
		
		pthread_mutex_unlock(&mutex);
		


		if(modo_texto) {
			pthread_cond_broadcast(&cond);
			sleep(TEMPO_NOVAREQ);
		}
		else
			// regra de 3 pegando o maior entre os 2 tempos e fazendo ele ser 500 ms, e diminuindo o outro proporcionalmente
			usleep(TEMPO_NOVAREQ > TEMPO_NOVOPROC ? 50000 : TEMPO_NOVAREQ*50000/TEMPO_NOVOPROC);
	}
	
	pthread_exit(NULL);	
}

// continuar a execucão (usuário pressionou espaco)
void continua() {
	
	pthread_mutex_lock(&mutex2);
	
	if(esperando) {
		esperando = 0;
		pthread_cond_signal(&cond2);
		pthread_mutex_unlock(&mutex2);
		return;
	}
	else
		pthread_mutex_unlock(&mutex2);

	pthread_mutex_lock(&mutex);
	ok = 1;
	pthread_mutex_unlock(&mutex);
	
	pthread_cond_broadcast(&cond);
}

void novo_processo() {
	
	int *t;
	t = malloc(sizeof(int));
	*t = proc_num;
	
	if(pthread_create(&tid[proc_num], NULL, thread_processo, t)) {
	    printf("Falha ao criar thread.\n");
        exit(1);
	}
	
	proc_num++;
}


void swap_in(int proc) {

	int i, j;
	
	msg3("[%02d:%02d] P%02d: vou voltar do swap", ultimo_tempo/60, ultimo_tempo%60, proc);
	
	
	
	// desenhando tabelas atuais
	desenha_tabelas(proc);
	
	pthread_mutex_lock(&mutex2);
	esperando = 1;
	// pausando para o usuário ver as tabelas antes da modificacão
	while(esperando) pthread_cond_wait(&cond2, &mutex2);
	
	pthread_mutex_unlock(&mutex2);
	

	int livres = 0; // frames livres na memoria
	int tamanho_ws = 0; // tamanho necessario pra copiar o WS atual (antes de adicionar)
	
	
	// calculando tamanho do working set
	for(i = 0; i < WORKING_SET; i++) {
		if(processos[proc].working_set[i] != -1) tamanho_ws++;
		else break;
	}
	
	// atualiza quantidade de livres
	livres = 0;
	for(i = 0; i < FRAMES; i++) {
		if(frames[i].processo == -1) livres++;
	}
	
	
	// enquanto o WS que for entrar não couber, vai vai tirando outros processos
	while(tamanho_ws > livres) {

		// swap out de alguém
		swap_out(-1); 

		// atualiza quantidade de frames livres
		livres = 0;
		for(i = 0; i < FRAMES; i++) {
			if(frames[i].processo == -1) livres++;
		}
	}

	
	// colocar efetivamente o WS dele na MP
	for(i = 0; i < WORKING_SET; i++) {

		if(processos[proc].working_set[i] == -1) break;
		else {
			// colocar no primeiro frame livre
			for(j = 0; j < FRAMES; j++) {
				if(frames[j].processo == -1) { // frame livre!
					frames[j].processo = proc;
					frames[j].pv = processos[proc].working_set[i];
					break;
				}
			}
		}
	}
	
	// alterar o numero de entrada pra saber que ele entrou agora
	processos[proc].entrada = num_entrada++;
	
	
	if(!modo_texto) {
		// desenhando tabelas atuais
		desenha_tabelas(proc);
	
		pthread_mutex_lock(&mutex2);
		esperando = 1;
		// pausando antes de continuar (antes de solicitar PV)
		while(esperando) pthread_cond_wait(&cond2, &mutex2);
	
		pthread_mutex_unlock(&mutex2);
	}
}


void solicita_pv(int proc, int num) {

	
	int i, j;
	
	int achou = 0;
	
	// processo voltando de swap
	if(processos[proc].entrada == INF && processos[proc].working_set[0] != -1) {
		swap_in(proc);
	}
	
	msg4("[%02d:%02d] P%02d: solicito PV%02d", ultimo_tempo/60, ultimo_tempo%60, proc, num);


	if(!modo_texto) {
		// desenhando tabelas atuais
		desenha_tabelas(proc);
	

		pthread_mutex_lock(&mutex2);
		esperando = 1;
		// pausando para o usuário ver as tabelas antes da modificacão
		while(esperando) pthread_cond_wait(&cond2, &mutex2);
		pthread_mutex_unlock(&mutex2);
	}
	
	for(i = 0; i < WORKING_SET; i++) {
		if(processos[proc].working_set[i] == num) {
			achou = 1;
			
			// colocar no final (LRU)	
			for(j = i; j < WORKING_SET-1; j++) {
				if(processos[proc].working_set[j+1] != -1) { // se não já for o último
					int temp = processos[proc].working_set[j];
					processos[proc].working_set[j] = processos[proc].working_set[j+1];
					processos[proc].working_set[j+1] = temp;
				}
			}
			
			break;
		}
	}
	
	if(achou) {
		msg("    └─ Já na MP!");
		
		// redesenhando tabelas da interface
		desenha_tabelas(proc);
	}
	
	// page fault!
	else {
		msg("    └─ Page fault!");
		adiciona_pv(proc, num);
		
		// redesenhando tabelas da interface
		desenha_tabelas(proc);
	}
}

// adiciona a pagina virtual num do processo proc à primeira posição livre da memória
// só deve ser chamada quando ocorre page fault (PV não está na memória)
int adiciona_pv(int proc, int num) {

	pthread_mutex_lock(&mp_mutex);
	
	int i, j;
	
	int ws_cheio = 1;
	int mp_cheia = 0;
	
	// ver se o WS está cheio
	for(i = 0; i < WORKING_SET; i++) {
		if(processos[proc].working_set[i] == -1) {
			ws_cheio = 0;
			break;
		}
	}
	
	
	// ver se o processo já está na memória (para alterar o num de entrada se estiver entrando agora)
	int na_memoria = 0;
	
	for(i = 0; i < FRAMES; i++) {
		if(frames[i].processo == proc) {
			na_memoria = 1;
			break;
		}
	}
	
	if(!na_memoria)
		processos[proc].entrada = num_entrada++;
	
	
	if(ws_cheio) {
	
		int pv_antiga = processos[proc].working_set[0];
		int pv_nova = num;
		int pos = -1;
		
		// alterações na MP
		
		// achar o antigo na MV e substituir
		for(j = 0; j < FRAMES; j++) {
			if(frames[j].processo == proc && frames[j].pv == pv_antiga) {
				frames[j].pv = pv_nova;
				pos = j;
				break;
			}
		}
		msg2("    └─ PV%02d alocada no frame %02d da MP", num, pos);
	
	
		// alterações no working set
		
		// o primeiro sai
		for(i = 0; i < WORKING_SET-1; i++) {
			processos[proc].working_set[i] = processos[proc].working_set[i+1];
		}
		
		// nova PV entra no final
		processos[proc].working_set[WORKING_SET-1] = num;
	}
	
	else {
		
		// alterações no working set
		for(j = 0; j < WORKING_SET; j++) {
			if(processos[proc].working_set[j] == -1) { // working set não está cheio ainda
				processos[proc].working_set[j] = num;
				break;
			}
		}
		
		int pos = -1;
	
		// alterações na MP
		for(i = 0; i < FRAMES; i++) {
			if(frames[i].processo == -1) { // frame livre!
				frames[i].processo = proc;
				frames[i].pv = num;
				pos = i;
				break;
			}
		}
		
		if(i == FRAMES) mp_cheia = 1;
		else msg2("    └─ PV%02d alocada no frame %02d da MP", num, pos);
	}

	// algum processo sofrerá swap out
	if(mp_cheia) {
		int frame_livre = swap_out(proc);
		
		// agora, podemos adicionar
		frames[frame_livre].processo = proc;
		frames[frame_livre].pv = num;
		
		msg2("    └─ PV%02d alocada no frame %02d da MP", num, frame_livre);
	}

	
	pthread_mutex_unlock(&mp_mutex);
	return 0;
	
}

int swap_out(int proc) {

	int i;
	
	int menor = INF;
	int quem = -1;
	
	// achando processo com menor num de entrada = entrou antes na memória
	for(i = 0; i < FRAMES; i++) {
		int n = frames[i].processo;
		if(n == -1 || n == proc) continue; // frame livre ou do próprio processo que está solicitando, não pode
		
		
		if(processos[n].entrada < menor) {
			quem = n;
			menor = processos[n].entrada;
		}
	}
	
	processos[quem].entrada = INF;

	int frame_livre = -1;
	
	// removendo todas as páginas desse processo da memória
	for(i = 0; i < FRAMES; i++) {
		if(frames[i].processo == quem) {
			frames[i] = (frame) {-1, -1};
			if(frame_livre == -1) frame_livre = i;
		}
	}
	
	msg1("    └─ Processo P%02d sofreu swap out!\n", quem);
	
	return frame_livre;
	
}
