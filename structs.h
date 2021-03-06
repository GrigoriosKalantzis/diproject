#ifndef STRUCTS_H_INCLUDED
#define STRUCTS_H_INCLUDED

#include <stdint.h>
#define BUFFSIZE 131072     //tuples that fit in 1MB buffer
#define N 10                //N last bits are hash key
#define THREADS 4

struct predicate{
    int flag;       //0 for join 1 for filter
    int t1;
    int c1;
    int t2;     //if join second table, if filter 0 for >, 1 for <, 2 for =
    int c2;     // if join second column, if filter constant number
};
typedef struct predicate Predicate;

struct tuple {
    int32_t key;
    int32_t payload;
};
typedef struct tuple Tuple;

struct relation {
    Tuple *tuples;
    uint32_t num_tuples;
};
typedef struct relation Relation;

struct matrix{
    uint64_t num_rows;
    uint64_t num_columns;
    uint64_t **columns;
    struct control *controls;
};
typedef struct matrix Matrix;

struct control{
    int I;
    int U;
    int F;
    int D;
};
typedef struct control Control;

struct index{
    Relation R;
    int *Chain;
    int *Bucket;
};
typedef struct index Index;

struct result {
    Tuple joins[BUFFSIZE];
    int counter;
    struct result *next;
};
typedef struct result Result;

struct histstruct{
    Relation *R;
    int startR;
    int endR;
    Relation *S;
    int startS;
    int endS;
    Relation *histR;
    Relation *histS;
};
typedef struct histstruct Histstruct;

void loadrelation(Matrix **matrixes, int matrixnum, char* fname);
char* execQuery(char query[], Matrix *matrixes, Index ***indexes);
void equalfilter(Control **controls, int column, int constant, int num_columns);
void unequalfilter(Control **controls, int column, int operand, int constant, int num_columns);
void selfcontrol(Control **controls1, Control **controls2, int column1, int column2, int num_columns1, int num_columns2);
void joincontrol(Control **controls1, Control **controls2, int column1, int column2, int num_columns1, int num_columns2);
void initrelation(Relation *rel, int rows, uint64_t *values);
int getrescount(Result *res);
Result* RadixHashJoin(Relation *relR, Relation *relS, Index **indexR, Index **indexS, int originalR, int originalS);
Result* Filter(Relation *rel, int operand, int constant);
Result* SelfJoin(Relation *relR, Relation *relS);
Relation initarray(Relation *rel, int *rowids, double buckets);
Relation inithist(Relation R, double buckets);
Relation initpsum(Relation hist, double buckets);
Index* createindex(Relation *Rel, Relation *psumR, double buckets, int h2margin);
Result* join(Index *ind, Relation *Rel, Relation *psumR, double buckets, int h2margin, int *Rrowids, int *Srowids, int flag);
int h1(int num, double n);
int h2(int num, int margin);
Result* createbuff(void);
void insertbuff(Result* res, int rowid1, int rowid2);
void copybuff(Result *res, int *table[], int flag);
void freebuff(Result *res);
void quicksort(Relation* rel,int first,int last, int* rowids);
int charcounter(char* str, char c);
void insertpred(char *s, Predicate *p, int index);


#endif // STRUCTS_H_INCLUDED
