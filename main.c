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

    int i,j;

    Matrix *matrixes;
    Index ***indexes;
    int matrixnum = 0;
    char filename[10];
    char *fname, *output;
    char query[100];
    char buffer[4096];


    matrixes = malloc(sizeof(Matrix));

    while(1){       //getting relation file names
        fgets(filename,10,stdin);
        if (strcmp(filename,"Done\n") == 0) break;

        matrixnum++;
        fname = strtok(filename,"\n");

        loadrelation(&matrixes,matrixnum,fname);

    }

    indexes = malloc(matrixnum * sizeof(Index*));       //index pointer array
    for(i = 0; i < matrixnum; i++){
        indexes[i] = malloc(matrixes[i].num_columns * sizeof(Index*));
        for(j = 0; j < matrixes[i].num_columns; j++){
            indexes[i][j] = NULL;
        }
    }


    buffer[0] = '\0';

    while(1){       //getting query batches
        if(fgets(query, 100, stdin) != NULL){

            if(strcmp(query,"End\n") == 0) break;

            if(strcmp(query,"F\n") == 0){
                write(1, buffer, strlen(buffer));
                buffer[0] = '\0';
                sleep(1);
                continue;
            }

            output = execQuery(query, matrixes, indexes);
            strcat(buffer, output);
            free(output);
        }
    }

    for(i = 0; i < matrixnum; i++){
        for(j = 0; j < matrixes[i].num_columns; j++){
            if(indexes[i][j] != NULL){
                free(indexes[i][j]->Bucket);
                free(indexes[i][j]->Chain);
                free(indexes[i][j]->R.tuples);
                free(indexes[i][j]);
            }
        }
        free(indexes[i]);
    }
    free(indexes);

    for(i = 0; i < matrixnum; i++){
        for(j = 0; j < matrixes[i].num_columns; j++){
            free(matrixes[i].columns[j]);
        }
        free(matrixes[i].columns);
        free(matrixes[i].controls);
    }
    free(matrixes);

    return 0;
}
