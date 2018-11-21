#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "structs.h"


Result* RadixHashJoin(Relation *relR, Relation *relS){

//number of final bits that the hash function will use
    int n = 8;
//margin of hash2 values
    int h2margin = 101;

    double buckets = pow(2.0, (double)n);

    int i;

    int* Rrowids;
    int* Srowids;
    Relation R,S;
    Relation histR, histS;
    Relation psumR, psumS;
    Relation *Min, *Max, *psumMin, *psumMax;

    Rrowids = malloc(relR->num_tuples * sizeof(int));
    Srowids = malloc(relS->num_tuples * sizeof(int));

//Initial arrays
    R = initarray(relR, Rrowids, buckets);
    S = initarray(relS, Srowids, buckets);

//Histograms
    histR = inithist(R, buckets);
    histS = inithist(S, buckets);

//Cumulative histograms
    psumR = initpsum(histR, buckets);
    psumS = initpsum(histS, buckets);

//Sorted buckets
    quicksort(&R, 0, R.num_tuples -1, Rrowids);     //sorting elements of both relations in buckets according to h1 value
    quicksort(&S, 0, S.num_tuples -1, Srowids);     //also sorting row ids so they coincide with their real value positions

//h2 values calculation
    for(i = 0; i < R.num_tuples; i++){
        R.tuples[i].key = h2(R.tuples[i].payload, h2margin);
    }
    for(i = 0; i < S.num_tuples; i++){
        S.tuples[i].key = h2(S.tuples[i].payload, h2margin);
    }

//Index build
    int flag;
    if(R.num_tuples < S.num_tuples){    //the index will be built for the smallest array
        Min = &R;
        Max = &S;
        psumMin = &psumR;
        psumMax = &psumS;
        flag = 0;
    }
    else{
        Min = &S;
        Max = &R;
        psumMin = &psumS;
        psumMax = &psumR;
        flag = 1;
    }

    Index *ind;

    ind = createindex(Min, psumMin, buckets, h2margin);


//Joins

    Result* res;

    res = join(ind, Max, psumMax, buckets, h2margin, Rrowids, Srowids, flag);



    free(Rrowids);
    free(Srowids);
    free(psumR.tuples);
    free(psumS.tuples);
    free(histR.tuples);
    free(histS.tuples);
    free(R.tuples);
    free(S.tuples);

    return res;
}

Relation initarray(Relation *rel, int *rowids, double buckets){

    int i;
    Relation R;

    R.num_tuples = rel->num_tuples;
    R.tuples = malloc(R.num_tuples * sizeof(Tuple));
    for(i = 0; i < R.num_tuples; i++){
        rowids[i] = rel->tuples[i].key;                           //array of row ids to be used when making the joins
        R.tuples[i].key = h1(rel->tuples[i].payload, buckets);     //h1 values of elements
        R.tuples[i].payload = rel->tuples[i].payload;              //real values
    }

    return R;
}

Relation inithist(Relation R, double buckets){
    int i,j;
    Relation hist;

    hist.num_tuples = buckets;
    hist.tuples = malloc(hist.num_tuples * sizeof(Tuple));

    for(i = 0; i < buckets; i++){
        hist.tuples[i].key = i;            //values of hash keys
        hist.tuples[i].payload = 0;        //initializing counter of each value
    }
    for(i = 0; i < R.num_tuples; i++){
        j = R.tuples[i].key;
        hist.tuples[j].payload++;      //increasing counter for each appearance of its hash value
    }

    return hist;
}

Relation initpsum(Relation hist, double buckets){
    int i;
    Relation psum;

    psum.num_tuples = buckets;
    psum.tuples = malloc(psum.num_tuples * sizeof(Tuple));

    for(i = 0; i < buckets; i++){
        psum.tuples[i].key = i;        //values of hash keys
    }

    psum.tuples[0].payload = 0;        //start of first bucket

    for(i = 1; i < buckets; i++){           //using the histograms we assign the start point of each bucket
        psum.tuples[i].payload = hist.tuples[i-1].payload + psum.tuples[i-1].payload;
    }

    return psum;
}

Index* createindex(Relation *Rel, Relation *psumR, double buckets, int h2margin){

    Index *ind;
    int i,j;

    ind = malloc(sizeof(Index));

    //copy R table
    ind->R.num_tuples = Rel->num_tuples;
    ind->R.tuples = malloc(Rel->num_tuples * sizeof(Tuple));

    for(i = 0; i < Rel->num_tuples + 1; i++){
        ind->R.tuples[i].key = Rel->tuples[i].key;
        ind->R.tuples[i].payload = Rel->tuples[i].payload;
    }

    //Chain array initialization
    ind->Chain = malloc((Rel->num_tuples + 1) * sizeof(int));
    for(i = 1; i < Rel->num_tuples + 1; i++){
        ind->Chain[i] = 0;
    }
    ind->Chain[0] = -1;

    //Bucket array initialization
    ind->Bucket = malloc(((int)buckets * h2margin) * sizeof(int));
    for(i = 0; i < ((int)buckets * h2margin); i++){
        ind->Bucket[i] = 0;
    }

    //index build
    int buckindex, chainindex;
    int currbottom = Rel->num_tuples;

    for(i = ((int)buckets-1); i >= 0; i--){     //starting from the bottom

        for(j = currbottom-1; j >= (psumR->tuples[i].payload); j--){      //each value of a bucket

            buckindex = i*h2margin + Rel->tuples[j].key;        //gets stored in the corresponding index
            if(ind->Bucket[buckindex] == 0){
                ind->Bucket[buckindex] = j+1;
            }
            else{
                chainindex = ind->Bucket[buckindex];
                while(ind->Chain[chainindex] != 0){
                    chainindex = ind->Chain[chainindex];
                }
                ind->Chain[chainindex] = j+1;
            }

        }
        currbottom = psumR->tuples[i].payload;
    }
    return ind;
}

Result* join(Index *ind, Relation *Rel, Relation *psumR, double buckets, int h2margin, int *Rrowids, int *Srowids, int flag){

    Result* res;
    int i,j;
    int buckindex, chainindex;
    int currbottom = Rel->num_tuples;

    res = createbuff();     //first buffer creation

    for(i = ((int)buckets-1); i >= 0; i--){

        for(j = currbottom-1; j >= (psumR->tuples[i].payload); j--){      //for each value of the greater array

            buckindex = i*h2margin + Rel->tuples[j].key;    //we search the corresponding index for matches
            if(ind->Bucket[buckindex] != 0){

                if(ind->R.tuples[ind->Bucket[buckindex]-1].payload == Rel->tuples[j].payload){
                    if(flag){
                        insertbuff(res, Rrowids[j] , Srowids[ind->Bucket[buckindex]-1]);      //if the real values match their row ids are inserted into the buffer
                    }
                    else{
                        insertbuff(res, Rrowids[ind->Bucket[buckindex]-1] , Srowids[j]);
                    }
                }
                chainindex = ind->Bucket[buckindex];
                while(ind->Chain[chainindex] != 0){
                     if(ind->R.tuples[ind->Chain[chainindex]-1].payload == Rel->tuples[j].payload){
                        if(flag){
                            insertbuff(res, Rrowids[j] , Srowids[ind->Chain[chainindex]-1]);
                        }
                        else{
                            insertbuff(res, Rrowids[ind->Chain[chainindex]-1] , Srowids[j]);
                        }
                    }
                    chainindex = ind->Chain[chainindex];
                }
            }

        }
        currbottom = psumR->tuples[i].payload;
    }

    return res;
}


int h1(int num, double n){

    return (num % (int)n);
}

int h2(int num, int margin){

    return ((num*2654435761) % margin);
}

Result* createbuff(void){
    Result* res;
    res = malloc(sizeof(Result));
    res->counter=0;                 //initialize element counter to 0
    res->next = NULL;           //and pointer to next buffer to NULL
    return res;
}


void insertbuff(Result* res, int rowid1, int rowid2){

    if(res->counter < BUFFSIZE){
        res->joins[res->counter].key = rowid1;
        res->joins[res->counter].payload = rowid2;
        res->counter++;                     //if buffer is not full insert element
    }
    else{
        if(res->next == NULL){
            res->next = createbuff();       //if next buffer does not exist create one
        }
        insertbuff(res->next, rowid1, rowid2);  //if buffer is full insert in next buffer
    }
}

void printbuff(Result *res, FILE *fp, long sum){
    int i;

    for(i=0; i<res->counter; i++){
        fprintf(fp, "%d\t%d\n", res->joins[i].key,res->joins[i].payload);
        sum++;
    }

    if(res->next != NULL){
        printbuff(res->next, fp, sum);
    }
    else{
        printf("TOTAL JOINS: %ld\n", sum);
    }

}

void freebuff(Result *res){
    if(res->next != NULL){
        freebuff(res->next);
    }

    free(res);
}


void quicksort(Relation* rel,int first,int last, int* rowids){
   int i, j, pivot;
   Tuple temp;
   int tempid;

   if(first<last){
      pivot=first;
      i=first;
      j=last;

      while(i<j){
         while(rel->tuples[i].key <= rel->tuples[pivot].key && i < last)
            i++;
         while(rel->tuples[j].key > rel->tuples[pivot].key)
            j--;
         if(i<j){
            temp.key = rel->tuples[i].key;
            temp.payload = rel->tuples[i].payload;
            tempid = rowids[i];
            rel->tuples[i].key = rel->tuples[j].key;
            rel->tuples[i].payload = rel->tuples[j].payload;
            rowids[i] = rowids[j];
            rel->tuples[j].key = temp.key;
            rel->tuples[j].payload = temp.payload;
            rowids[j] = tempid;
         }
      }

      temp.key = rel->tuples[pivot].key;
      temp.payload = rel->tuples[pivot].payload;
      tempid = rowids[pivot];
      rel->tuples[pivot].key = rel->tuples[j].key;
      rel->tuples[pivot].payload =rel->tuples[j].payload;
      rowids[pivot] = rowids[j];
      rel->tuples[j].key =temp.key;
      rel->tuples[j].payload = temp.payload;
      rowids[j] = tempid;

      quicksort(rel,first,j-1, rowids);
      quicksort(rel,j+1,last, rowids);

   }
}

int charcounter(char* str, char c){
    int counter = 0;
    int i;

    for(i = 0; i<strlen(str); i++){
        if (str[i] == c) counter++;
    }

    return counter;
}

void insertpred(char *s, Predicate *p, int index){

    int equals, dots;
    char *str;

    equals = charcounter(s,'=');

    if(equals == 1){

        dots = charcounter(s,'.');
        if(dots == 1){
            p[index].flag = 1;
            p[index].t1 = s[0] - '0';
            p[index].c1 = s[2] - '0';
            p[index].t2 = 2;
            strtok(s, "=");
            str = strtok(NULL, "=");
            p[index].c2 = atoi(str);
        }
        else if(dots == 2){
            p[index].flag = 0;
            p[index].t1 = s[0] - '0';
            p[index].c1 = s[2] - '0';
            p[index].t2 = s[4] - '0';
            p[index].c2 = s[6] - '0';
        }

    }
    else if(equals == 0){
        p[index].flag = 1;
        p[index].t1 = s[0] - '0';
        p[index].c1 = s[2] - '0';

        if(charcounter(s, '>') == 1){
            p[index].t2 = 0;
            strtok(s, ">");
            str = strtok(NULL, ">");
            p[index].c2 = atoi(str);
        }
        else{
            p[index].t2 = 1;
            strtok(s, "<");
            str = strtok(NULL, "<");
            p[index].c2 = atoi(str);
        }
    }
}
