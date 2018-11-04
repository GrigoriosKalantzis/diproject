#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "structs.h"


int main(void){

    clock_t start_t, end_t, total_t;
    int i;
    FILE *fp;

    int margin = 10000;

    start_t = clock();

    /*int R[] = {54,32,93,165,87,122,234,90,12,342,4,5,6,7,2,3,4,1,8,0,2,4,5,6,7,2,3,4,1,8,0,2};
    int Rrows = sizeof(R)/sizeof(int);
    int S[] = {11,73,93,86,54,65,93,112,5,1,9,7,3,12,8,0};
    int Rrows = sizeof(S)/sizeof(int);*/

    srand(time(NULL));
    int Rrows = 300000;
    int Srows = 200000;
    int R[Rrows];
    int S[Srows];
    for(i = 0; i < Rrows; i++){
        R[i] = rand() % margin;
    }
    for(i = 0; i < Srows; i++){
        S[i] = rand() % margin;
    }


    Relation relR,relS;
    Result* res;

//making tuples with join columns and row ids
    relR.num_tuples = Rrows;
    relR.tuples = malloc(relR.num_tuples * sizeof(Tuple));
    for(i = 0; i < Rrows; i++){
        relR.tuples[i].key = i+1;
        relR.tuples[i].payload = R[i];
    }

    relS.num_tuples = Srows;
    relS.tuples = malloc(relS.num_tuples * sizeof(Tuple));
    for(i = 0; i < Srows; i++){
        relS.tuples[i].key = i+1;
        relS.tuples[i].payload = S[i];
    }

//Join
    res = RadixHashJoin(&relR, &relS);

//printing results

    fp = fopen("output.txt", "w");

    fprintf(fp,"JOINS\n\nRrows\tSrows\n");
    printbuff(res, fp, 0);
    freebuff(res);

    end_t = clock();

    total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
    printf("Total time taken by CPU in seconds: %ld\n", total_t  );
    printf("Clock Ticks: %ld\n", (end_t - start_t));


    return 0;
}
