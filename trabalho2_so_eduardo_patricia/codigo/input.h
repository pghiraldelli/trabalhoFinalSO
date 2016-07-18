#include <termios.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#ifndef COMUM
#define COMUM
#include "comum.h"
#endif


extern int mem_inicio;
extern int tp_inicio;
extern void atualiza_tp(int proc);
extern void atualiza_memoria();
extern void fim();
extern int lin;
extern int col;
extern int ultimo_proc;

extern pthread_mutex_t print_mutex;
extern pthread_mutex_t mp_mutex;

extern void desenha_tabelas(int proc);

extern void continua();

void initTermios(int echo);
void resetTermios(void);
char getch();

void *thread_input(void *arg);
