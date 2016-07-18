#include "interface.h"

int mem_inicio = 0; // mostrar a tabela de memória a partir desse índice (scroll)
int tp_inicio = 0; // mostrar a tabela de páginas virtuais a partir desse índice


// captura de sinais
void sig_handler(int signo) {
	if(signo == SIGINT) { // ctrl+c
		fim();
	}
	if(signo == SIGWINCH) // resize do terminal
		fim();
}

// imprime mensagem de erro e termina a execução
void erro(char *str) {
	printf("%s", str);
	exit(1);
}

// altura (número de linhas) do terminal
int linhas() {
	int lin;

	FILE *fp = popen("tput lines", "r");	
	if(fp == NULL) erro("Falha ao rodar o tput para descobrir o tamanho do terminal.\n");

	fscanf(fp, "%d", &lin);
	
	return lin;
}

// largura (número de colunas) do terminal
int colunas() {
	int col;

	FILE *fp = popen("tput cols", "r");
	if(fp == NULL) erro("Falha ao rodar o tput para descobrir o tamanho do terminal.\n");
	
	fscanf(fp, "%d", &col);
	
	return col;
}

// itens iniciais básicos da interface, que não precisam ser atualizados futuramente
void interface_inicio() {

	
	// tamanho da janela do terminal
	col = colunas();
	lin = linhas();
	
	if(col == 0) col = 80;
	if(lin == 0) lin = 24;


	pthread_mutex_lock(&print_mutex);
	
	printf("\033c"); // "resetar" terminal (limpar de verdade, previnindo scroll acidental)
	printf("\033[2J"); // limpar tela
	printf("\e[?25l"); // ocultar cursor

	// tabela da memória ("\033[%d;%dH" da esquerda = movendo o cursor para imprimir na posicao certa)
	printf("\e[44m");
	printf("\033[%d;%dH", 1,     col-MEM_OFFSET);   printf("\e[1;93;104m   Memória Principal   \e[0;97;44m");
	printf("\033[%d;%dH", 2,     col-MEM_OFFSET);   printf(" ╔═══════╤═══════════╗ ");
	printf("\033[%d;%dH", 3,     col-MEM_OFFSET);   printf(" ║ Frame │ Conteúdo  ║ ");
	printf("\033[%d;%dH", 4,     col-MEM_OFFSET);   printf(" ╟───────┼───────────╢ ");
	printf("\033[%d;%dH", lin-1, col-MEM_OFFSET); printf(" ╚═══════╧═══════════╝ ");
	printf("\e[0m");
	
	atualiza_proc_num(0);
	
	// tabela de paginas
	printf("\e[41;97m");
	printf("\033[%d;%dH", 2,     col-TP_OFFSET); printf("  Tabela de Pág:   \e[0;41;97m");
	printf("\033[%d;%dH", 3,     col-TP_OFFSET); printf(" ╔════╤══════════╗ ");
	printf("\033[%d;%dH", 4,     col-TP_OFFSET); printf(" ║ PV │ Endereço ║ ");
	printf("\033[%d;%dH", 5,     col-TP_OFFSET); printf(" ╟────┼──────────╢ ");
	printf("\033[%d;%dH", lin-6, col-TP_OFFSET);  printf(" ╚════╧══════════╝ ");
	printf("\e[0m");
	
	// working set
	printf("\e[41;97m");
	printf("\033[%d;%dH  Working Set:     \e[0;41;97m", lin-4,   col-TP_OFFSET);
	printf("\033[%d;%dH ╔═══════════════╗ ", lin-3,   col-TP_OFFSET);
	printf("\033[%d;%dH ║ 01 02 03 04   ║ ", lin-2,   col-TP_OFFSET);
	printf("\033[%d;%dH ╚═══════════════╝ ", lin-1, col-TP_OFFSET);
	printf("\e[0m");
	
	// contador de tempo
	printf("\033[%d;%dHTempo = ", lin-2, 0);
	
	// linha debaixo com as teclas e suas respectivas ações
	char *teclas = "Q = Sair    Espaço = Próx Evento    A/Z = Scroll (TP)    Setas = Scroll (MP)";
	int sobrando = col - 76;
	int espaco_esq = sobrando/2;
	int espaco_dir = sobrando/2;
	if(sobrando % 2 == 1) espaco_dir++;
	
	printf("\033[%d;%dH", lin, 0);
	printf("\e[0;47;90m");
	printf("%*s%s%*s", espaco_esq, "", teclas, espaco_dir, "");
	printf("\e[0m");
	
	pthread_mutex_unlock(&print_mutex);
	
	// mostrar o relogio e a memória inicialmente
	atualiza_hora();
	atualiza_memoria();
	atualiza_tp(0);
	atualiza_ws(0);
	
	signal(SIGINT, sig_handler);
	
}

void interface_fim() {

	resetTermios(); // resetar atributos modificados pela funcao de input (não mostrar caracteres digitados e não esperar o enter)
	
	printf("\e[?25h"); // mostrar cursor
	printf("\033c"); // "resetar" terminal
	printf("\033[2J"); // limpar tela
	
	fflush(stdout);
}

// atualização do relógio
void atualiza_hora() {

	
	pthread_mutex_lock(&print_mutex);

	printf("\033[%d;%dH%02d:%02d", lin-2, 9, t.m, t.s);
	fflush(stdout);

	pthread_mutex_unlock(&print_mutex);
	
	t.s++;
	if(t.s >= 60) {
		t.s = 0;
		t.m++;
	}
}

void atualiza_hora2(int seg) {

	pthread_mutex_lock(&print_mutex);

	printf("\033[%d;%dH%02d:%02d", lin-2, 9, seg/60, seg%60);
	fflush(stdout);

	pthread_mutex_unlock(&print_mutex);
}



// atualização da tabela da memória, com os frames e conteúdos
void atualiza_memoria() {
	
	int i;
	
	printf("\e[44;97m");
	
	for(i = 0; i < lin-6; i++) {
		int f = mem_inicio + i;
		
		printf("\033[%d;%dH", 5+i,   col-MEM_OFFSET);
		
		// completar parte sem nada (conteúdo da tabela menor que a altura da janela)
		if(f >= FRAMES) {
			printf(" ║       │           ║ ");
			continue;
		}
		
		printf(" ║ %-5d │ ", f);
		

		// frame livre
		if(frames[f].processo == -1)
			printf("-         ║ ");
		
		else
			printf("P%02d, PV%02d ║ ", frames[f].processo, frames[f].pv);
	}
	
	int mem_de = mem_inicio;
	int mem_ate = mem_inicio + (lin-6-1);
	
	// mostrar setas quando não tá no início nem final
	if(mem_de > 0) {
		printf("\033[%d;%dH ╟───────┴───────────╢ ", 4,   col-MEM_OFFSET);
		printf("\033[%d;%dH ║\e[93m      ▲  ▲  ▲      \e[0;44;97m║ ", 5, col-MEM_OFFSET);
	}
	else {
		printf("\033[%d;%dH ╟───────┼───────────╢ ", 4,   col-MEM_OFFSET);
	}
	
	if(mem_ate < FRAMES-1) {
		printf("\033[%d;%dH ║\e[93m      ▼  ▼  ▼      \e[0;44;97m║ ", lin-2, col-MEM_OFFSET);
		printf("\033[%d;%dH ╚═══════════════════╝ ", lin-1, col-MEM_OFFSET);
	}
	else {
		printf("\033[%d;%dH ╚═══════╧═══════════╝ ", lin-1, col-MEM_OFFSET);
	}
	
	printf("\e[0m");
	
	fflush(stdout);
	
}


void desenha_tabelas(int proc) {

	pthread_mutex_lock(&print_mutex);
	
	ultimo_proc = proc;
	
	
	if(modo_texto) {
		imprime_memoria();
		imprime_tp(proc);
		imprime_ws(proc);
	}
	else {
		atualiza_memoria();
		atualiza_proc_num(proc);
		atualiza_tp(proc);
		atualiza_ws(proc);
	}
	
	pthread_mutex_unlock(&print_mutex);
}



void imprime_memoria() {

	int i;
	int meio = FRAMES/2;
	
	printf("\n");
	printf("    *********************\n");
	printf("    * Memória Principal *\n");
	printf("    *********************");
	
	printf("\n    Frame ");
	for(i = 0; i < meio; i++)
		printf("%02d ", i);
	
	printf("\n    Proc  ");
	for(i = 0; i < meio; i++) {
		if(frames[i].processo == -1) printf("-  ");
		else printf("%02d ", frames[i].processo);
	}
	
	printf("\n    PV    ");
	for(i = 0; i < meio; i++) {
		if(frames[i].processo == -1) printf("-  ");
		else printf("%02d ", frames[i].pv);
	}
	
	printf("\n\n    Frame ");
	for(i = meio; i < FRAMES; i++)
		printf("%02d ", i);
	
	printf("\n    Proc  ");
	for(i = meio; i < FRAMES; i++) {
		if(frames[i].processo == -1) printf("-  ");
		else printf("%02d ", frames[i].processo);
	}
	
	printf("\n    PV    ");
	for(i = meio; i < FRAMES; i++) {
		if(frames[i].processo == -1) printf("-  ");
		else printf("%02d ", frames[i].pv);
	}
	
	
	printf("\n\n");
}


void imprime_tp(int proc) {
	int i, j;
	
	printf("    ****************************\n");
	printf("    * Tabela de páginas de P%02d *\n", proc);
	printf("    ****************************");
	printf("\n    PV   ");
	for(i = 0; i < PAGINAS_VIRTUAIS; i++)
		printf("%02d ", i);
	
	printf("\n    End* ");
	for(i = 0; i < PAGINAS_VIRTUAIS; i++) {

		int tem = 0;
		
		for(j = 0; j < FRAMES; j++) {
			if(frames[j].processo == proc && frames[j].pv == i) {
				tem = 1;
				break;
			}
		}
		
		// página virtual está na memória
		if(tem)
			printf("%02d ", j);
		else
			printf("-  ");
	}
	
	printf("\n    *(número do frame onde a PV está)\n\n");
}

void imprime_ws(int proc) {
	int i;
	
	printf("    **********************\n");
	printf("    * Working Set de P%02d *\n", proc);
	printf("    **********************");
	
	printf("\n    ");
	for(i = 0; i < WORKING_SET; i++) { // só funciona com 4, por enquanto
		if(processos[proc].working_set[i] == -1 || processos[proc].entrada == INF) printf("-  ");
		else printf("%02d ", processos[proc].working_set[i]);
	}
	
	printf("\n\n");
}


void atualiza_proc_num(int proc) {
	printf("\e[1;32;101m");
	printf("\033[%d;%dH", 1, col-TP_OFFSET);
	printf("   Processo P%02d    ", proc);
	printf("\e[0m");
	
	fflush(stdout);
	
}

// desenha a tabela de paginas e a lista de paginas solicitadas do processo proc
void atualiza_tp(int proc) {

	
	int i, j;
	
	printf("\e[41;97m");
	
	//printf("\033[%d;%dH", 1,   col-TP_OFFSET); printf("\e[1;36m Tabela do P%02d:\e[0m", proc);
	
	for(i = 0; i < lin-11; i++) {
		int p = tp_inicio + i;
		
		printf("\033[%d;%dH", 6+i,   col-TP_OFFSET);
	
		// completar parte sem nada (conteúdo da tabela menor que a altura da janela)
		if(p >= PAGINAS_VIRTUAIS) {
			printf(" ║    │          ║ ");
			continue;
		}
	
		printf(" ║ %-2d │ ", p);

		
		int tem = 0;
		
		for(j = 0; j < FRAMES; j++) {
			if(frames[j].processo == proc && frames[j].pv == p) {
				tem = 1;
				break;
			}
		}
		
		// página virtual está na memória
		if(tem)
			printf("frame %02d ║ ", j);
		else
			printf("-        ║ ");
	}
	
	
	int tp_de = tp_inicio;
	int tp_ate = tp_inicio + (lin-11-1);
	
	// mostrar setas quando não tá no início nem final
	if(tp_de > 0) {
		printf("\033[%d;%dH ╟────┴──────────╢ ", 5, col-TP_OFFSET);
		printf("\033[%d;%dH ║\e[92m    ▲  ▲  ▲    \e[0;41;97m║ ", 6, col-TP_OFFSET);
	}
	else {
		printf("\033[%d;%dH ╟────┼──────────╢ ", 5,   col-TP_OFFSET);
	}
	
	if(tp_ate < PAGINAS_VIRTUAIS-1) {
		printf("\033[%d;%dH ║\e[92m    ▼  ▼  ▼    \e[0;41;97m║ ", lin-6, col-TP_OFFSET);
		printf("\033[%d;%dH ╚═══════════════╝ ", lin-5, col-TP_OFFSET);
	}
	else {
		printf("\033[%d;%dH ╚════╧══════════╝ ", lin-5, col-TP_OFFSET);
	}
	
	
	printf("\e[0m");
	
	fflush(stdout);
}


#define MAX_MSG 500

char mensagens[MAX_MSG][50]; // MAX_MSG mensagens de 50 caracteres no max
int msgnum = 0;
int ciclo = 0; // 0 se ainda não foi concluído nenhum ciclo na array de mensagens

void msg(char *msg) {
	pthread_mutex_lock(&print_mutex);
	
	if(modo_texto) {
		printf("%s", msg);
		printf("\n");
	}
		
	else {
		sprintf(mensagens[msgnum], "%s", msg);
		if(ciclo == 0 && msgnum == MAX_MSG - 1) ciclo = 1;
		msgnum = (msgnum + 1) % MAX_MSG;
		atualiza_msg();
	}
	
	pthread_mutex_unlock(&print_mutex);
}

void msg1(char *msg, int arg1) {
	pthread_mutex_lock(&print_mutex);
	
	if(modo_texto) {
		printf(msg, arg1);
		printf("\n");
	}
	
	else {
		sprintf(mensagens[msgnum], msg, arg1);
		if(ciclo == 0 && msgnum == MAX_MSG - 1) ciclo = 1;
		msgnum = (msgnum + 1) % MAX_MSG;
		atualiza_msg();
	}
	
	pthread_mutex_unlock(&print_mutex);
}

void msg2(char *msg, int arg1, int arg2) {
	pthread_mutex_lock(&print_mutex);
	
	if(modo_texto) {
		printf(msg, arg1, arg2);
		printf("\n");
	}
	
	else {
		sprintf(mensagens[msgnum], msg, arg1, arg2);
		if(ciclo == 0 && msgnum == MAX_MSG - 1) ciclo = 1;
		msgnum = (msgnum + 1) % MAX_MSG;
		atualiza_msg();
	}
	
	pthread_mutex_unlock(&print_mutex);
}

void msg3(char *msg, int arg1, int arg2, int arg3) {
	pthread_mutex_lock(&print_mutex);
	
	if(modo_texto) {
		if(msg[0] == '[') printf("\n"); // espaço antes de mensagens do tipo "[MM:SS] Evento"
		printf(msg, arg1, arg2, arg3);
		printf("\n");
	}
	
	else {
		sprintf(mensagens[msgnum], msg, arg1, arg2, arg3);
		if(ciclo == 0 && msgnum == MAX_MSG - 1) ciclo = 1;
		msgnum = (msgnum + 1) % MAX_MSG;
		atualiza_msg();
	}
	
	pthread_mutex_unlock(&print_mutex);
}

void msg4(char *msg, int arg1, int arg2, int arg3, int arg4) {
	pthread_mutex_lock(&print_mutex);
	
	if(modo_texto) {
		if(msg[0] == '[') printf("\n"); // espaço antes de mensagens do tipo "[MM:SS] Evento"
		printf(msg, arg1, arg2, arg3, arg4);
		printf("\n");
	}
	
	else {
		sprintf(mensagens[msgnum], msg, arg1, arg2, arg3, arg4);
		if(ciclo == 0 && msgnum == MAX_MSG - 1) ciclo = 1;
		msgnum = (msgnum + 1) % MAX_MSG;
		atualiza_msg();
	}
	
	pthread_mutex_unlock(&print_mutex);
}

void atualiza_msg() {

	
	int espaco = lin - 4;
	if(espaco > MAX_MSG) espaco = MAX_MSG;
	
	int num = msgnum;
	if(ciclo == 1) num += MAX_MSG; // já deu um ciclo no vetor de mensagem = tem MAX_MSG mensagens
	
	int offset = num - espaco;
	if(offset < 0) offset = 0;

	int i;
	
	for(i = 0; i < espaco; i++) {
		if(i >= num) break;
		printf("\033[%d;%dH%38s", i+1, 0, "");
		printf("\033[%d;%dH%s", i+1, 0, mensagens[(i+offset) % MAX_MSG]);
	}
	
	fflush(stdout);
}


void atualiza_ws(int proc) {

	
	printf("\e[41;97m");
	
	printf("\033[%d;%dH ║ ", lin-2,   col-TP_OFFSET);
	
	int i;
	
	for(i = 0; i < WORKING_SET; i++) { // só funciona com 4, por enquanto
		if(processos[proc].working_set[i] == -1 || processos[proc].entrada == INF) printf("-  ");
		else printf("%02d ", processos[proc].working_set[i]);
	}
	
	printf("  ║");
	
	printf("\e[0m");
	
	fflush(stdout);
	
}

void *thread_relogio(void *arg) {

	while(1) {
		atualiza_hora();
		sleep(1);
	}
	
	pthread_exit(NULL);
}
