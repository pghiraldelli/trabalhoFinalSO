
#define FRAMES           64
#define PROCESSOS        20
#define PAGINAS_VIRTUAIS 50
#define WORKING_SET      4


#define TEMPO_NOVAREQ    3
#define TEMPO_NOVOPROC   3

#define INF 0x7FFFFFFF

// tempo, com minutos e segundos, para guardar o relógio
typedef struct tempo {
	int m;
	int s;
} tempo;

// frame da memória, que contem uma PV de um processo
typedef struct frame {
	int processo;
	int pv;
} frame;

// processo (representado por threads no trabalho)
typedef struct processo {
	int tabela[PAGINAS_VIRTUAIS];
	int working_set[WORKING_SET];
	int entrada; // momento em que o processo entrou na MP
} processo;

