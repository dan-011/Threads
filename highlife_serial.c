#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>

#define DEFAULT_FILE_NAME    "start0.txt"
#define DEFAULT_CYCLES       100
#define DEFAULT_PRINT_FLAG   1

#define REFRESH_MILLISEC 1000/30

void msleep(int tms)
{
    struct timeval tv;
    tv.tv_sec  = tms / 1000;
    tv.tv_usec = (tms % 1000) * 1000;
    select (0, NULL, NULL, NULL, &tv);
}


typedef struct {
   int    row;
   int    col;
   char** src;
} Board;

Board* makeBoard(int r, int c)
{
   Board* p = malloc(sizeof(Board));
   p->row   = r;
   p->col   = c;
   p->src   = malloc(sizeof(char*)*r);
   for(int i=0; i<r; i++)
      p->src[i] = malloc(sizeof(char)*c);
   return p;
}
void freeBoard(Board* b)
{
   for(int i=0; i<b->row; i++)
      free(b->src[i]);
   free(b->src);
   free(b);
}

Board* readBoard(char* fName)
{
   int row, col;
   FILE* src = fopen(fName,"r");
   fscanf(src, "%d %d\n", &row, &col);
   Board* rv = makeBoard(row, col);
   for(int i=0; i<row; i++) {
      for(int j=0; j<col; j++) {
         char ch = fgetc(src);
         rv->src[i][j] = ch == '*';
      }
      char skip = fgetc(src);
      while (skip != '\n') skip = fgetc(src);
   }
   fclose(src);
   return rv;
}

void saveBoard(Board* b, FILE* fd)
{
   for(int i=0; i<b->row; i++) {
      fprintf(fd,"|");
      for(int j=0; j < b->col; j++)
         fprintf(fd,"%c",b->src[i][j] ? '*' : ' ');
      fprintf(fd,"|\n");
   }
}

void printBoard(Board* b)
{
   printf("\033\143");  /* this is to clear the terminal */
   saveBoard(b,stdout);
}

int liveNeighbors(int i, int j, Board* b)
{
   const int pc = (j-1) < 0 ? b->col-1 : j - 1;
   const int nc = (j + 1) % b->col;
   const int pr = (i-1) < 0 ? b->row-1 : i - 1;
   const int nr = (i + 1) % b->row;
   int xd[8] = {pc , j , nc, pc, nc, pc , j , nc };
   int yd[8] = {pr , pr, pr, i , i , nr , nr ,nr };
   int ttl = 0;
   for(int k=0; k < 8; k++)
      ttl += b->src[yd[k]][xd[k]];
   return ttl;
}

void evolveBoard(Board* in, Board* out)
{
   static int rule[2][9] = {
      {0,0,0,1,0,0,1,0,0},
      {0,0,1,1,0,0,0,0,0}
   };
   for(int i=0; i < in->row; i++) {
      for(int j=0; j < in->col; j++) {
         int ln = liveNeighbors(i, j, in);
         int c  = in->src[i][j];
         out->src[i][j] = rule[c][ln];
      }
   }
}

int main(int argc, char* argv[])
{
   char* file_name  = (argc < 2 ? DEFAULT_FILE_NAME : argv[1] );
   int cycles      = (argc < 3 ? DEFAULT_CYCLES : atoi(argv[2]) );
   int printFlag   = (argc < 4 ? DEFAULT_PRINT_FLAG : atoi(argv[3]) );

   if (argc < 2) {
       fprintf(stderr, "Usage: %s <start_file> <cycles> <print_flag>\n", argv[0]);
       exit(1);
   }

   Board* board1 = readBoard(file_name);
   Board* board2 = makeBoard(board1->row, board1->col);
   Board* b[2] = {board1, board2};

   for(int g=0; g < cycles; g++) {
      if (printFlag) {
          printBoard(b[g%2]);
          msleep(REFRESH_MILLISEC);
      }
      evolveBoard(b[g%2], b[1-g%2]); // this is where threads are called
      // wait after calling (using barrier)
   }
   if (printFlag)
       printBoard(b[1-cycles%2]);

   FILE* final = fopen("final_serial.txt","w");
   saveBoard(b[1-cycles%2], final);
   fclose(final);
   freeBoard(board1);
   freeBoard(board2);
   return 0;
}
