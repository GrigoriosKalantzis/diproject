#ifndef STRUCTS_H_INCLUDED
#define STRUCTS_H_INCLUDED

#include <stdint.h>
#define BUFFSIZE 131072     //tuples that fit in 1MB buffer

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
};
typedef struct matrix Matrix;

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

void loadrelation(Matrix *matrixes, int matrixnum, char* fname);
char* execQuery(char query[], Matrix *matrixes);
void initrelation(Relation *rel, int rows, uint64_t *values);
int getrescount(Result *res);
Result* RadixHashJoin(Relation *relR, Relation *relS);
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
void printbuff(Result *res, FILE *fp, long sum);
void freebuff(Result *res);
void quicksort(Relation* rel,int first,int last, int* rowids);
int charcounter(char* str, char c);
void insertpred(char *s, Predicate *p, int index);


#endif // STRUCTS_H_INCLUDED
