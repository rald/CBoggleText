#define _GNU_SOURCE
#include <ncurses.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#define TIME_MAX 3*60
#define DICT_FILE "WordList.txt"
#define BOARD_SIZE 4
#define STRING_MAX 256
#define LETTERS_MAX 26
#define WORD_LENGTH_MIN 3
#define WORD_LENGTH_MAX BOARD_SIZE*BOARD_SIZE

char board[BOARD_SIZE][BOARD_SIZE];
bool graph[BOARD_SIZE][BOARD_SIZE];

char *dice[]= {
    "AAEEGN",
    "ABBJOO",
    "ACHOPS",
    "AFFKPS",
    "AOOTTW",
    "CIMOTU",
    "DEILRX",
    "DELRVY",
    "DISTTY",
    "EEGHNW",
    "EEINSU",
    "EHRTVW",
    "EIOSST",
    "ELRTTY",
    "HIMNUQ",
    "HLNNRZ"
};

int numDice=16;

bool quit=false;

double starttime,currenttime;
double elapsedtime=0,remainingtime=0,allottedtime=TIME_MAX;


char word[BOARD_SIZE*BOARD_SIZE+1];
char **words=NULL;
int numWords=0;
bool *guessed=NULL;
int numGuessed=0;
int score=0;
bool gameover=false;

void addWord(char ***words,int *numWords,char *word) {
    (*words)=realloc(*words,sizeof(char*)*((*numWords)+1));
    (*words)[(*numWords)++]=strdup(word);
}


typedef struct Trie Trie;

struct Trie {
    bool mark;
    Trie *next[LETTERS_MAX];
};

Trie *root;

Trie *Trie_New() {
    Trie *trie=malloc(sizeof(Trie));
    int i;
    if(trie) {
        trie->mark=false;
        for(i=0; i<LETTERS_MAX; i++) {
            trie->next[i]=NULL;
        }
    }
    return trie;
}



void Trie_Add(Trie *root,char *word) {
    Trie *curr_node=root;
    char *curr_let=word;
    int index;

    while(*curr_let) {

        if(tolower(*curr_let)<'a' || tolower(*curr_let)>'z') {
            printw("Error: Invalid character in word '%s'.\n",word);
            exit(1);
        }

        index=tolower(*curr_let)-'a';

        if(curr_node->next[index]==NULL) {
            curr_node->next[index]=Trie_New();
        }

        curr_node=curr_node->next[index];

        curr_let++;
    }

    curr_node->mark=true;
}



Trie *loadDict(char *path) {
    Trie *root=Trie_New();
    FILE *fin;
    char line[STRING_MAX];
    fin=fopen(path,"rt");
    while(fgets(line,STRING_MAX,fin)) {
        char *p=strchr(line,'\n');
        if(p) *p='\0';
        if(	strlen(line)>=WORD_LENGTH_MIN &&
                strlen(line)<=WORD_LENGTH_MAX) {
            Trie_Add(root,line);
        }
    }
    return root;
}



int indexOf(char **words,int numWords,char *word) {
    int i;
    for(i=0; i<numWords; i++) {
        if(!strcmp(words[i],word)) return i;
    }
    return -1;
}



void dfs(int x,int y,int depth,Trie *root) {

    char ch;
    int index;

    int i,j;

    if(x<0 || x>=BOARD_SIZE || y<0 || y>=BOARD_SIZE) return;

    if(graph[y][x]) return;

    ch=tolower(board[y][x]);

    index=ch-'a';

    root=root->next[index];

    if(!root) return;

    word[depth]=ch;
    word[depth+1]='\0';

    if(	root->mark &&
            depth+1>=WORD_LENGTH_MIN &&
            indexOf(words,numWords,word)==-1) {
        addWord(&words,&numWords,word);
    }

    graph[y][x]=true;

    for(j=-1; j<=1; j++) {
        for(i=-1; i<=1; i++) {
            if (i || j) dfs(x+i,y+j,depth+1,root);
        }
    }

    graph[y][x]=false;
}



void shuffle(char *dice[],int numDice) {
    int i;
    for(i=numDice-1; i>0; i--) {
        int j=rand()%(i+1);
        char *tmp=dice[i];
        dice[i]=dice[j];
        dice[j]=tmp;
    }
}



void setupBoard(char board[BOARD_SIZE][BOARD_SIZE],char *dice[]) {
    int i,j,k;
    k=0;
    for(j=0; j<BOARD_SIZE; j++) {
        for(i=0; i<BOARD_SIZE; i++) {
            board[j][i]=toupper(dice[k][rand()%strlen(dice[k])]);
            k++;
        }
    }
}


void printBoard(char board[BOARD_SIZE][BOARD_SIZE]) {
    int i,j;
    printw("\n");
    for(j=0; j<BOARD_SIZE; j++) {
        for(i=0; i<BOARD_SIZE; i++) {
            printw("%c ",board[j][i]);
        }
        printw("\n");
    }
}


void printWords(char **words,int numWords) {
    int i;
    bool first=true;
    printw("\n");
    for(i=0; i<numWords; i++) {
        if(gameover || guessed[i]) {
            if(!first) printw(", ");
            else first=false;
            if(guessed[i]) {
                printw("%s",words[i]);
            } else if(gameover) {
                printw("[%s]",words[i]);
            }
        }
    }
    printw("\n");
}


int cmpAscByLen(const void *a,const void *b) {
    char *l=*(char**)a;
    char *r=*(char**)b;
    int ll=strlen(l);
    int rl=strlen(r);

    if(ll<rl) {
        return -1;
    } else if(ll>rl) {
        return 1;
    }

    return strcmp(l,r);
}


void getWords() {
    int i,j;

    for (j=0; j<BOARD_SIZE; j++) {
        for (i=0; i<BOARD_SIZE; i++) {
            graph[j][i]=false;
        }
    }

    for (j=0; j<BOARD_SIZE; j++) {
        for (i=0; i<BOARD_SIZE; i++) {
            dfs(i,j,0,root);
        }
    }

    qsort(words,numWords,sizeof(char*),cmpAscByLen);
}

void initGame() {
    int i;

    shuffle(dice,numDice);
    setupBoard(board,dice);

    if(numWords>0) {
        for(i=0; i<numWords; i++) {
            free(words[i]);
            words[i]=NULL;
        }
        free(words);
        words=NULL;
        numWords=0;
    }

    getWords();

    numGuessed=0;
    guessed=malloc(sizeof(bool)*numWords);
    for(i=0; i<numWords; i++) {
        guessed[i]=false;
    }

    score=0;

    printBoard(board);

    starttime=time(NULL);

    gameover=false;

}

void printPrompt() {
    printw("\n> ");
}

int main() {

    int x,y;

    int ch;

    char *str=NULL;

    int maxx,maxy;

    int min,sec;

    char line[STRING_MAX];
    int len=0;

    int index;

    int points;


    srand(time(NULL));

    root=loadDict(DICT_FILE);

    initscr();

    cbreak();
    noecho();
    nodelay(stdscr,TRUE);
    keypad(stdscr,TRUE);
    scrollok(stdscr,TRUE);

    initGame();

    printw("\nEnter '.help' for list of commands.\n");

    printPrompt();

    while(!quit) {

        currenttime=time(NULL);
        elapsedtime=difftime(currenttime,starttime);
        remainingtime=allottedtime-elapsedtime;
        if(remainingtime<=0) {
            remainingtime=0;
            if(!gameover) {
                printw("\nTime's up.");
                printPrompt();
                gameover=true;
            }
        }

        min=(int)floor(remainingtime/60);
        sec=(int)fmod(remainingtime,60);

        getyx(stdscr,y,x);
        asprintf(&str,"%d:%02d",min,sec);
        getmaxyx(stdscr,maxy,maxx);
        move(0,maxx-strlen(str));
        printw("%s",str);
        move(y,x);

        ch=getch();
        if(ch!=ERR) {
//            printf("%i\n",ch);
            if(ch==27) {
                quit=true;
            } else if(ch==263) {
                if(len>0) {
                    line[--len]='\0';
                    addch(8);
                    addch(32);
                    addch(8);
                }
            } else if(ch==10) {

                printw("\n");

                if(!strcmp(line,".help")) {
                    printw(
                        "--- commands ---\n\n"
                        "[Esc] -> exit\n"
                        ".help -> print this help\n"
                        ".clear -> clear screen\n"
                        ".board -> print board\n"
                        ".list -> list words\n"
                        ".score -> print score\n"
                        ".start -> start game"
                    );
                } else if(!strcmp(line,".clear")) {
                    clear();
                } else if(!strcmp(line,".board")) {
                    printBoard(board);
                } else if(!strcmp(line,".list")) {
                    printWords(words,numWords);
                } else if(!strcmp(line,".score")) {
                    printw("Your score is %d.",score);
                } else if(!strcmp(line,".start")) {
                    if(gameover) {
                        initGame();
                    } else {
                        printw("Game already started.");
                    }
                } else if(!gameover) {
                    index=indexOf(words,numWords,line);
                    if(index!=-1) {
                        if(!guessed[index]) {
                            guessed[index]=true;
                            numGuessed++;

                            len=strlen(words[index]);

                            if(len<=4) points=1;
                            else if(len==5) points=2;
                            else if(len==6) points=3;
                            else if(len==7) points=5;
                            else if(len>7) points=11;

                            score+=points;

                            printw("You've guessed '%s' plus %d point(s) score %d.",words[index],points,score);
                        } else {
                            printw("The word '%s' is already guessed.",words[index]);
                        }
                    } else {
                        printw("The word '%s' is not found.",line);
                    }
                } else {
                    printw("The game is over.");
                }

                printw("\n> ");
                len=0;
                line[0]='\0';
            } else {
                if(len<STRING_MAX-1) {
                    if((toupper(ch)>='A' && toupper(ch)<='Z') || (ch=='.' && len==0)) {
                        line[len++]=ch;
                        line[len]='\0';
                        addch(ch);
                    }
                }
            }
        }

        refresh();

        free(str);
    }

    refresh();

    endwin();

    return 0;

}

