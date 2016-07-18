// capturar tecla pressionada sem exibir o caractere
// fonte: http://stackoverflow.com/a/7469410

#include "input.h"


static struct termios old_, new_;

/* Initialize new terminal i/o settings */
void initTermios(int echo) 
{
  tcgetattr(0, &old_); /* grab old terminal i/o settings */
  new_ = old_; /* make new settings same as old settings */
  new_.c_lflag &= ~ICANON; /* disable buffered i/o */
  new_.c_lflag &= echo ? ECHO : ~ECHO; /* set echo mode */
  tcsetattr(0, TCSANOW, &new_); /* use these new terminal i/o settings now */
}

/* Restore old terminal i/o settings */
void resetTermios() 
{
  tcsetattr(0, TCSANOW, &old_);
}

/* Read 1 character without echo */
char getch() 
{
  char ch;
  initTermios(0);
  ch = getchar();

  if(ch == 27) { // setas direcionais são 3 caracteres (27 91 65 a seta pra cima, por ex)
	  ch = getchar();
	  ch = getchar();
	  ch -= 65;
  }

  resetTermios();
  return ch;
}


void *thread_input(void *arg) {


	while(1) {
		char c = getch();
		
		//printf("\033[1;1HTecla %d    \n", c);
		
		switch(c) {
			// scroll up na memória
			case 0: // seta para cima
				if(mem_inicio > 0) mem_inicio--;
				desenha_tabelas(ultimo_proc);
				break;
			
			// scroll down na memória
			case 1: // seta para baixo
				if(mem_inicio < FRAMES-(lin-6)) mem_inicio++;
				desenha_tabelas(ultimo_proc);
				break;
			
			// sair
			case 113: // q
			case 81:  // Q
				fim();
				break;
			
			// scroll up na TP
			case 97:  // a
			case 65:  // A
				if(tp_inicio > 0) tp_inicio--;
				desenha_tabelas(ultimo_proc);
				break;
			
			// scroll down na TP
			case 90:  // z
			case 122: // Z
				if(tp_inicio < PAGINAS_VIRTUAIS-(lin-11)) tp_inicio++;
				desenha_tabelas(ultimo_proc);
				break;
				
			// espaço
			case 32:
				continua();
		}
		

	}

	pthread_exit(NULL);
}
