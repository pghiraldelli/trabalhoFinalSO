#define _XOPEN_SOURCE
//http://stackoverflow.com/a/5468444
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>

#ifndef COMUM
#define COMUM
#include "comum.h"
#endif

#define MEM_OFFSET 22
#define TP_OFFSET 41

extern int lin, col;
extern tempo t;
extern frame frames[];
extern processo processos[];

extern pthread_mutex_t print_mutex;
extern pthread_mutex_t mp_mutex;

extern int ultimo_proc;

extern int modo_texto;

extern void fim();

extern void resetTermios();

void erro(char *str);
int linhas();
int colunas();
void interface_inicio();
void atualiza_hora();
void atualiza_memoria();
void mostra_tp();
void atualiza_tp(int proc);
void atualiza_ws(int proc);
void atualiza_proc_num(int proc);
void interface_fim();
void atualiza_hora2(int seg);
void desenha_tabelas(int proc);
void *thread_relogio(void *arg);

void imprime_memoria();
void imprime_tp(int proc);
void imprime_ws(int proc);

void msg(char *msg);
void msg1(char *msg, int arg1);
void msg2(char *msg, int arg1, int arg2);
void msg3(char *msg, int arg1, int arg2, int arg3);
void msg4(char *msg, int arg1, int arg2, int arg3, int arg4);
void atualiza_msg();
