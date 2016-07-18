#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#ifndef COMUM
#define COMUM
#include "comum.h"
#endif

extern frame frames[];
extern processo processos[];
extern pthread_t tid[];

extern void atualiza_memoria();
extern void mostra_tp();
extern void atualiza_tp(int proc);
extern void atualiza_ws(int proc);

extern void msg(char *msg);
extern void msg1(char *msg, int arg1);
extern void msg2(char *msg, int arg1, int arg2);
extern void msg3(char *msg, int arg1, int arg2, int arg3);
extern void msg4(char *msg, int arg1, int arg2, int arg3, int arg4);

extern pthread_mutex_t mp_mutex;

extern void desenha_tabelas(int proc);


extern pthread_mutex_t mutex;
extern pthread_cond_t cond;

extern pthread_mutex_t mutex2;
extern pthread_cond_t cond2;

extern int ok;
extern int ultimo_proc;

extern int modo_texto;

extern int ticket;
extern int prox_ticket;

extern void atualiza_hora2(int seg);

void *thread_processo(void *arg);
void novo_processo();
int adiciona_pv(int proc, int num);

int swap_out(int proc);
void swap_in(int proc);


void solicita_pv(int proc, int num);

void continua();
