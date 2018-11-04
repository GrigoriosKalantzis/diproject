#ifndef STRUCTS_H_INCLUDED
#define STRUCTS_H_INCLUDED

#include <stdint.h>
#define BUFFSIZE 131072     //tuples that fit in 1MB buffer

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

struct result {
    Tuple joins[BUFFSIZE];
    int counter;
    struct result *next;
};
typedef struct result Result;

Result* RadixHashJoin(Relation *relR, Relation *relS);
int h1(int num, double n);
int h2(int num, int margin);
Result* createbuff(void);
void insertbuff(Result* res, int rowid1, int rowid2);
void printbuff(Result *res, FILE *fp, long sum);
void freebuff(Result *res);
void quicksort(Relation* rel,int first,int last, int* rowids);


#endif // STRUCTS_H_INCLUDED
