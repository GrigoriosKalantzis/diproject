#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include "structs.h"
#include "sys/stat.h"
#include "fcntl.h"
#include "unistd.h"

int main(void){


    int margin = 10000;
    clock_t start_t, end_t, total_t;
    int i;
    FILE *fp;

    Matrix *matrixes;
    int matrixnum = 0;
    char filename[10];
    char *fname;

    start_t = clock();

    matrixes = malloc(sizeof(Matrix));

    while(1){
        fgets(filename,10,stdin);
        if (strcmp(filename,"Done\n") == 0) break;

        matrixnum++;
        fname = strtok(filename,"\n");

        loadrelation(matrixes,matrixnum,fname);

    }


    srand(time(NULL));
    int Rrows = 300;
    int Srows = 200;
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

    fclose(fp);

    free(relR.tuples);
    free(relS.tuples);

    end_t = clock();

    total_t = (double)(end_t - start_t) / CLOCKS_PER_SEC;
    printf("Total time taken by CPU in seconds: %ld\n", total_t  );
    printf("Clock Ticks: %ld\n", (end_t - start_t));


    return 0;
}
