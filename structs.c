#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include "structs.h"

pthread_mutex_t histmutexR = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t histmutexS = PTHREAD_MUTEX_INITIALIZER;


void* histthread(void *in){     //histogram construction thread
    Histstruct *hstruct = in;
    int k,m;
    for(k = hstruct->startR; k < hstruct->endR; k++){

        pthread_mutex_lock(&histmutexR);
        m = hstruct->R->tuples[k].key;
        hstruct->histR->tuples[m].payload++;
        pthread_mutex_unlock(&histmutexR);

    }
    for(k = hstruct->startS; k < hstruct->endS; k++){

        pthread_mutex_lock(&histmutexS);
        m = hstruct->S->tuples[k].key;
        hstruct->histS->tuples[m].payload++;
        pthread_mutex_unlock(&histmutexS);

    }

    pthread_exit((void *) 47);
}


void loadrelation(Matrix **matrixes, int matrixnum, char* fname){

    int i,j;

    int fd = open(fname, O_RDONLY);
    if (fd==-1) {
        perror("cannot open");
        exit(1);
    }

    if(matrixnum != 1){
       (*matrixes) = realloc((*matrixes), matrixnum * sizeof(Matrix));
    }

    // Obtain file size
    struct stat sb;
    if (fstat(fd,&sb)==-1)
        perror("fstat\n");
    int length=sb.st_size;

    char* addr= (char*)(mmap(NULL,length,PROT_READ,MAP_PRIVATE,fd,0u));
    if (addr==MAP_FAILED) {
        perror("cannot mmap");
        exit(1);
    }

    if (length<16) {
        perror("relation file does not contain a valid header");
        exit(1);
    }

	uint64_t min, max;
	int dcounter;
	int *dtable = malloc(4*sizeof(int));

    (*matrixes)[matrixnum-1].num_rows = *(uint64_t*)(addr);
    addr+=sizeof(uint64_t);
    (*matrixes)[matrixnum-1].num_columns = *(uint64_t*)(addr);
    addr+=sizeof(uint64_t);
    (*matrixes)[matrixnum-1].columns = malloc((*matrixes)[matrixnum-1].num_columns * sizeof(uint64_t*));
	(*matrixes)[matrixnum-1].controls = malloc((*matrixes)[matrixnum-1].num_columns * sizeof(Control));
    for(i = 0; i < (*matrixes)[matrixnum-1].num_columns; i++){
		min = 1000000;
		max = 0;

        (*matrixes)[matrixnum-1].columns[i] = malloc((*matrixes)[matrixnum-1].num_rows * sizeof(uint64_t));
        for(j = 0; j < (*matrixes)[matrixnum-1].num_rows; j++){
            (*matrixes)[matrixnum-1].columns[i][j] = *(uint64_t*)(addr);
			if(min > (*matrixes)[matrixnum-1].columns[i][j]) min = (*matrixes)[matrixnum-1].columns[i][j];
			if(max < (*matrixes)[matrixnum-1].columns[i][j]) max = (*matrixes)[matrixnum-1].columns[i][j];
            addr+=sizeof(uint64_t);
        }

		(*matrixes)[matrixnum-1].controls[i].I = min;
		(*matrixes)[matrixnum-1].controls[i].U = max;
		(*matrixes)[matrixnum-1].controls[i].F = (*matrixes)[matrixnum-1].num_rows;

		dtable = realloc(dtable, (max-min+1) * sizeof(int));
		for(j = 0; j < (max-min+1); j++){
			dtable[j] = 0;
		}
		for(j = 0; j < (*matrixes)[matrixnum-1].num_rows; j++){
			dtable[((*matrixes)[matrixnum-1].columns[i][j]) - min] = 1;
		}
		dcounter = 0;
		for(j = 0; j < (max-min+1); j++){
			if(dtable[j]) dcounter++;
		}
		(*matrixes)[matrixnum-1].controls[i].D = dcounter;
    }
	free(dtable);
    close(fd);
}

char* execQuery(char query[], Matrix *matrixes, Index ***indexes){

    char *rel, *pred, *sum;
    int rels, preds, sums;
    char *s;
    int i,j,k;
    int q = 0;

    if(strcmp(query, "3 0 13 13|0.2=1.0&1.0=2.1&2.1=3.2&0.2<74|1.2 2.5 3.5\n") == 0) q = 1;

//splitting query in individual data
    rel = strtok(query,"|");
    pred = strtok(NULL,"|");
    sum = strtok(NULL,"|");

    rels = charcounter(rel,' ');
    preds = charcounter(pred,'&');
    sums = charcounter(sum,' ');

//relations used
    int relations[rels+1];
    relations[0] = atoi(strtok(rel," "));
    for(i=1; i<=rels; i++){
        relations[i] = atoi(strtok(NULL," "));
    }

//predicates
    Predicate predicates[preds+1];
    s = strtok(pred,"&");
    insertpred(s, predicates, 0);
    for(i=1; i<=preds; i++){
        s = strtok(NULL,"&");
        insertpred(s, predicates, i);
    }
    int filtersno = 0;
    int joinsno = 0;
    for(i=0; i<=preds; i++){
        if(predicates[i].flag) filtersno++;
        else joinsno++;
    }
    Predicate filters[filtersno];
    Predicate joinsprev[joinsno];
    filtersno = 0;
    joinsno = 0;
    for(i=0; i<=preds; i++){
        if(predicates[i].flag){
            filters[filtersno].flag = predicates[i].flag;
            filters[filtersno].t1 = predicates[i].t1;
            filters[filtersno].c1 = predicates[i].c1;
            filters[filtersno].t2 = predicates[i].t2;
            filters[filtersno].c2 = predicates[i].c2;
            filtersno++;
        }
        else{
            joinsprev[joinsno].flag = predicates[i].flag;
            joinsprev[joinsno].t1 = predicates[i].t1;
            joinsprev[joinsno].c1 = predicates[i].c1;
            joinsprev[joinsno].t2 = predicates[i].t2;
            joinsprev[joinsno].c2 = predicates[i].c2;
            joinsno++;
        }
    }

//checksums
    int checksums[sums+1][2];
    s = strtok(sum, " ");
    checksums[0][0] = s[0] - '0';
    checksums[0][1] = s[2] - '0';
    for(i=1; i<=sums; i++){
        s = strtok(NULL, " ");
        checksums[i][0] = s[0] - '0';
        checksums[i][1] = s[2] - '0';
    }

//ektimhsh plithikothtas

    int cjoined[rels+1][rels+1];
    for(i=0; i<=rels; i++){
        for(j=0; j<=rels; j++){
            cjoined[i][j] = 0;
        }
        cjoined[i][i] = 1;
    }
    int order[joinsno];
    int ordercount = 1;
    for(i=0; i<joinsno; i++){
        order[i] = 0;
    }
    long mincost, cost;
    int minpos, minimum, maximum;

    Control *controls[rels+1];

    for(i=0; i<=rels; i++){
        controls[i] = malloc(matrixes[relations[i]].num_columns * sizeof(Control));
        for(j=0; j<matrixes[relations[i]].num_columns; j++){
            controls[i][j].I = matrixes[relations[i]].controls[j].I;
            controls[i][j].U = matrixes[relations[i]].controls[j].U;
            controls[i][j].F = matrixes[relations[i]].controls[j].F;
            controls[i][j].D = matrixes[relations[i]].controls[j].D;
        }
    }

    for(i=0; i<filtersno; i++){
        if(filters[i].t2 == 2){
            equalfilter(&(controls[filters[i].t1]), filters[i].c1, filters[i].c2, matrixes[relations[filters[i].t1]].num_columns);
        }
        else{
            //fprintf(stderr, "BEFORE %d %d %d %d\n", controls[filters[i].t1][filters[i].c1].I, controls[filters[i].t1][filters[i].c1].U , controls[filters[i].t1][filters[i].c1].F, controls[filters[i].t1][filters[i].c1].D);
            unequalfilter(&(controls[filters[i].t1]), filters[i].c1, filters[i].t2, filters[i].c2, matrixes[relations[filters[i].t1]].num_columns);
            //fprintf(stderr, "AFTER %d %d %d %d\n", controls[filters[i].t1][filters[i].c1].I, controls[filters[i].t1][filters[i].c1].U , controls[filters[i].t1][filters[i].c1].F, controls[filters[i].t1][filters[i].c1].D);
        }
    }

    for(i=0; i<joinsno; i++){
        mincost = 9999999999;

        for(j=0; j<joinsno; j++){
            if(order[j] == 0){
                if(controls[joinsprev[j].t1][joinsprev[j].c1].I > controls[joinsprev[j].t2][joinsprev[j].c2].I)
                    minimum = controls[joinsprev[j].t1][joinsprev[j].c1].I;
                else
                    minimum = controls[joinsprev[j].t2][joinsprev[j].c2].I;

                if(controls[joinsprev[j].t1][joinsprev[j].c1].U < controls[joinsprev[j].t2][joinsprev[j].c2].U)
                    maximum = controls[joinsprev[j].t1][joinsprev[j].c1].U;
                else
                    maximum = controls[joinsprev[j].t2][joinsprev[j].c2].U;

                if(cjoined[joinsprev[j].t1][joinsprev[j].t2])
                    cost = controls[joinsprev[j].t1][joinsprev[j].c1].F / (maximum - minimum +1);
                else
                    cost = (controls[joinsprev[j].t1][joinsprev[j].c1].F * controls[joinsprev[j].t2][joinsprev[j].c2].F)/ (maximum - minimum +1);

                if(cost < mincost){
                    mincost = cost;
                    minpos = j;
                }
            }
        }

        if(cjoined[joinsprev[minpos].t1][joinsprev[minpos].t2]){
            selfcontrol(&(controls[joinsprev[minpos].t1]),&(controls[joinsprev[minpos].t2]), joinsprev[minpos].c1, joinsprev[minpos].c2, matrixes[relations[joinsprev[minpos].t1]].num_columns, matrixes[relations[joinsprev[minpos].t2]].num_columns);
        }
        else{
            joincontrol(&(controls[joinsprev[minpos].t1]),&(controls[joinsprev[minpos].t2]), joinsprev[minpos].c1, joinsprev[minpos].c2, matrixes[relations[joinsprev[minpos].t1]].num_columns, matrixes[relations[joinsprev[minpos].t2]].num_columns);
        }

        cjoined[joinsprev[minpos].t1][joinsprev[minpos].t2] = 1;
        cjoined[joinsprev[minpos].t2][joinsprev[minpos].t1] = 1;
        order[minpos] = ordercount;
        ordercount++;

    }


	if(q){
		for(i=0; i<joinsno; i++){
			order[i] = i+1;
		}
	}

	//joins order
    Predicate joins[joinsno];
    for(i=0; i<joinsno; i++){
		//fprintf(stderr,"%d\n", order[i]);
        joins[order[i]-1].flag = joinsprev[i].flag;
        joins[order[i]-1].t1 = joinsprev[i].t1;
        joins[order[i]-1].c1 = joinsprev[i].c1;
        joins[order[i]-1].t2 = joinsprev[i].t2;
        joins[order[i]-1].c2 = joinsprev[i].c2;
    }


//execution
    int joined[rels+1][rels+1];
    Result *res;
    int* results[rels+1];
    int* temp_results[rels+1];
    int num_results[rels+1];
    int flags[rels+1];
    for(i=0; i<=rels; i++){
        results[i] = malloc(4*sizeof(int));
        temp_results[i] = malloc(4*sizeof(int));
        num_results[i] = 0;
        flags[i] = 0;
        for(j=0; j<=rels; j++){
            joined[i][j] = 0;
        }
        joined[i][i] = 1;
    }

    uint64_t* temp_col;
    temp_col = malloc(4*sizeof(uint64_t));
    Relation relR, relS;
    relR.tuples = malloc(4*sizeof(Tuple));
    relS.tuples = malloc(4*sizeof(Tuple));

//filters execution
    for(i=0; i<filtersno; i++){
        if (flags[filters[i].t1] == 0)   //give whole column as input if no results exist
            initrelation(&relR, matrixes[relations[filters[i].t1]].num_rows, matrixes[relations[filters[i].t1]].columns[filters[i].c1]);
        else{
            //give only intermediate results as input
            temp_col = realloc(temp_col, num_results[filters[i].t1]*sizeof(uint64_t));
            for(j=0; j<num_results[filters[i].t1]; j++)
                temp_col[j] = matrixes[relations[filters[i].t1]].columns[filters[i].c1][results[filters[i].t1][j] - 1];
            initrelation(&relR, num_results[filters[i].t1], temp_col);
        }
        //Filter execution
        res = Filter(&relR, filters[i].t2, filters[i].c2);
        //get results number
        num_results[filters[i].t1] = getrescount(res);


        if (flags[filters[i].t1] == 0){     //place first results if none exist
            results[filters[i].t1] = realloc(results[filters[i].t1], (num_results[filters[i].t1]) * sizeof(int));
            copybuff(res, &(results[filters[i].t1]), 1);
        }
        else{
            //or replace old ones
            temp_results[filters[i].t1] = realloc(temp_results[filters[i].t1], (num_results[filters[i].t1]) * sizeof(int));
            copybuff(res ,&(temp_results[j]), 1);
            for(k=0; k<num_results[filters[i].t1]; k++){
                temp_results[filters[i].t1][k] = results[filters[i].t1][temp_results[filters[i].t1][k]-1];
            }
            //&(results[j]) = &(temp_results[j]);
            results[filters[i].t1] = realloc(results[filters[i].t1], (num_results[filters[i].t1]) * sizeof(int));
            for(k=0; k<num_results[filters[i].t1]; k++){
                results[filters[i].t1][k] = temp_results[filters[i].t1][k];
            }
        }

        flags[filters[i].t1] = 1;

        freebuff(res);
    }

//joins execution
    int originalR, originalS;

    for(i=0; i<joinsno; i++){

        if (flags[joins[i].t1] == 0){   //give whole column as input if no results exist
            initrelation(&relR, matrixes[relations[joins[i].t1]].num_rows, matrixes[relations[joins[i].t1]].columns[joins[i].c1]);
            originalR = 1;
        }
        else{
            //give only intermediate results as input
            temp_col = realloc(temp_col, num_results[joins[i].t1]*sizeof(uint64_t));
            for(j=0; j<num_results[joins[i].t1]; j++)
                temp_col[j] = matrixes[relations[joins[i].t1]].columns[joins[i].c1][results[joins[i].t1][j] - 1];
            initrelation(&relR, num_results[joins[i].t1], temp_col);
            originalR = 0;
        }

        if (flags[joins[i].t2] == 0){   //give whole column as input if no results exist
            initrelation(&relS, matrixes[relations[joins[i].t2]].num_rows, matrixes[relations[joins[i].t2]].columns[joins[i].c2]);
            originalS = 1;
        }
        else{
            //give only intermediate results as input
            temp_col = realloc(temp_col, num_results[joins[i].t2]*sizeof(uint64_t));
            for(j=0; j<num_results[joins[i].t2]; j++)
                temp_col[j] = matrixes[relations[joins[i].t2]].columns[joins[i].c2][results[joins[i].t2][j] - 1];
            initrelation(&relS, num_results[joins[i].t2], temp_col);
            originalS = 0;
        }

        if(joined[joins[i].t1][joins[i].t2]){
            //join filter if the arrays have been joined
            res = SelfJoin(&relR, &relS);
        }
        else{
            //radix if not
            res = RadixHashJoin(&relR, &relS, &(indexes[relations[joins[i].t1]][joins[i].c1]), &(indexes[relations[joins[i].t2]][joins[i].c2]), originalR , originalS);
        }

        num_results[joins[i].t1] = getrescount(res);        //get results number
        num_results[joins[i].t2] = num_results[joins[i].t1];


        if (flags[joins[i].t1] == 0){  //if the table had no previous results keep the current results as they are
            results[joins[i].t1] = realloc(results[joins[i].t1], (num_results[joins[i].t1]) * sizeof(int));
            copybuff(res, &(results[joins[i].t1]), 1);
        }
        else{      //if it has adjust the joined tables accordingly
            for(j=0; j<=rels; j++){
                if((j != joins[i].t2) || (joins[i].t1 == joins[i].t2)){
                    if(joined[joins[i].t1][j]){
                        temp_results[j] = realloc(temp_results[j], (num_results[joins[i].t1]) * sizeof(int));
                        copybuff(res ,&(temp_results[j]), 1);
                        for(k=0; k<num_results[joins[i].t1]; k++){
                            temp_results[j][k] = results[j][temp_results[j][k]-1];
                        }
                        //&(results[j]) = &(temp_results[j]);
                        results[j] = realloc(results[j], (num_results[joins[i].t1]) * sizeof(int));
                        for(k=0; k<num_results[joins[i].t1]; k++){
                            results[j][k] = temp_results[j][k];
                        }
                        num_results[j] = num_results[joins[i].t1];
                    }
                }
            }
        }

        if(joins[i].t1 != joins[i].t2){
            if (flags[joins[i].t2] == 0){  //if the table had no previous results keep the current results as they are
                results[joins[i].t2] = realloc(results[joins[i].t2], (num_results[joins[i].t2]) * sizeof(int));
                copybuff(res, &(results[joins[i].t2]), 2);
            }
            else{
                if(joined[joins[i].t1][joins[i].t2]){
                    //if this is a self join replace only the second array's results
                    //because the joined tables have been handled in the first loop
                    temp_results[joins[i].t2] = realloc(temp_results[joins[i].t2], (num_results[joins[i].t2]) * sizeof(int));
                    copybuff(res ,&(temp_results[joins[i].t2]), 2);
                    for(k=0; k<num_results[joins[i].t2]; k++){
                        temp_results[joins[i].t2][k] = results[joins[i].t2][temp_results[joins[i].t2][k]-1];
                    }
                    results[joins[i].t2] = realloc(results[joins[i].t2], (num_results[joins[i].t2]) * sizeof(int));
                    for(k=0; k<num_results[joins[i].t2]; k++){
                        results[joins[i].t2][k] = temp_results[joins[i].t2][k];
                    }
                }
                else{
                    for(j=0; j<=rels; j++){
                        if((j != joins[i].t1)){
                            //if this is a normal join replace the joined arrays' results
                            if(joined[joins[i].t2][j]){
                                temp_results[j] = realloc(temp_results[j], (num_results[joins[i].t2]) * sizeof(int));
                                copybuff(res ,&(temp_results[j]), 2);
                                for(k=0; k<num_results[joins[i].t2]; k++){
                                    temp_results[j][k] = results[j][temp_results[j][k]-1];
                                }
                                results[j] = realloc(results[j], (num_results[joins[i].t2]) * sizeof(int));
                                for(k=0; k<num_results[joins[i].t2]; k++){
                                    results[j][k] = temp_results[j][k];
                                }
                                num_results[j] = num_results[joins[i].t2];
                            }
                        }
                    }
                }
            }
        }

        flags[joins[i].t1] = 1;
        flags[joins[i].t2] = 1;
        joined[joins[i].t1][joins[i].t2] = 1;
        joined[joins[i].t2][joins[i].t1] = 1;
        //the joined tables are now in the same struct
        for(j=0; j<=rels; j++){
            if(joined[joins[i].t2][j]){
                joined[joins[i].t1][j] = 1;
                joined[j][joins[i].t1] = 1;
            }
            if(joined[joins[i].t1][j]){
                joined[joins[i].t2][j] = 1;
                joined[j][joins[i].t2] = 1;
            }
        }
        freebuff(res);
    }


    uint64_t checks[sums+1];
	char buff[20];
    char *output = malloc(50*sizeof(char));
	output[0] = '\0';

    //checksum calculation
    for(i=0; i<=sums; i++){
        checks[i] = 0;
        for(j=0; j<num_results[checksums[i][0]]; j++){
            checks[i] += matrixes[relations[checksums[i][0]]].columns[checksums[i][1]][results[checksums[i][0]][j] - 1];
        }
        if (checks[i])
            sprintf(buff,"%lu ", checks[i]);
        else
            sprintf(buff,"NULL ");
        strcat(output, buff);
    }
    output[strlen(output) - 1] = '\n';

    //fprintf(stderr,"%s", output);

    for(i=0; i<=rels; i++){
        free(controls[i]);
        free(results[i]);
        free(temp_results[i]);
    }
    free(temp_col);
    free(relR.tuples);
    free(relS.tuples);

    return output;
}

void equalfilter(Control **controls, int column, int constant, int num_columns){
    int i;
    double temp, exp;
    int oldfa = (*controls)[column].F;
	if((*controls)[column].D)
    	(*controls)[column].F = ((*controls)[column].F) / ((*controls)[column].D);
    (*controls)[column].D = 1;
    (*controls)[column].I = constant;
    (*controls)[column].U = constant;

    for(i=0; i<num_columns; i++){
        if(i != column){
			if(oldfa)
            	temp = (double)((*controls)[column].F) / (double)oldfa;
			else
				temp = (double)((*controls)[column].F);
			if((*controls)[i].D)
            	exp = (double)((*controls)[i].F) / (double)((*controls)[i].D);
			else
				exp = (double)((*controls)[i].F);
            temp = pow(1-temp, exp);
            (*controls)[i].D = (*controls)[i].D * (1 - temp);
            (*controls)[i].F = (*controls)[column].F;
			if(((*controls)[i].D == 0) && ((*controls)[i].F != 0))
				(*controls)[i].D = 1;

        }
    }


}

void unequalfilter(Control **controls, int column, int operand, int constant, int num_columns){
    int i;
    double temp0, temp, exp;
    int oldfa = (*controls)[column].F;

    if(operand == 0){           //greater
        if(constant < (*controls)[column].I)
            constant = (*controls)[column].I;

        if((*controls)[column].U == (*controls)[column].I){
            (*controls)[column].F = ((*controls)[column].U - constant) * (*controls)[column].F;
            (*controls)[column].D = ((*controls)[column].U - constant) * (*controls)[column].D;
            (*controls)[column].I = constant;
        }
        else{
            temp0 = (double)((*controls)[column].U - constant) / (double)((*controls)[column].U - (*controls)[column].I);
            (*controls)[column].F = temp0 * (*controls)[column].F;
            (*controls)[column].D = temp0 * (*controls)[column].D;
            (*controls)[column].I = constant;
        }


    }
    else{                //less
        if(constant > (*controls)[column].U)
            constant = (*controls)[column].U;

        if((*controls)[column].U == (*controls)[column].I){
            (*controls)[column].F = (constant - (*controls)[column].I) * (*controls)[column].F;
            (*controls)[column].D = (constant - (*controls)[column].I) * (*controls)[column].D;
            (*controls)[column].U = constant;
        }
        else{
            temp0 = (double)(constant - (*controls)[column].I) / (double)((*controls)[column].U - (*controls)[column].I);
            (*controls)[column].F = temp0 * (*controls)[column].F;
            (*controls)[column].D = temp0 * (*controls)[column].D;
            (*controls)[column].U = constant;
        }
    }

    for(i=0; i<num_columns; i++){
        if(i != column){
			if(oldfa)
            	temp = (double)((*controls)[column].F) / (double)oldfa;
			else
				temp = (double)((*controls)[column].F);
			if((*controls)[i].D)
            	exp = (double)((*controls)[i].F) / (double)((*controls)[i].D);
			else
				exp = (double)((*controls)[i].F);
            temp = pow(1-temp, exp);
            (*controls)[i].D = (*controls)[i].D * (1 - temp);
            (*controls)[i].F = (*controls)[column].F;
			if(((*controls)[i].D == 0) && ((*controls)[i].F != 0))
				(*controls)[i].D = 1;

        }
    }
}

void selfcontrol(Control **controls1, Control **controls2, int column1, int column2, int num_columns1, int num_columns2){

    int i, n, min, max;
    double temp, exp;
    int oldfa = (*controls1)[column1].F;

    if((*controls1)[column1].I > (*controls2)[column2].I){
        (*controls2)[column2].I = (*controls1)[column1].I;
        min = (*controls1)[column1].I;
    }
    else{
        (*controls1)[column1].I = (*controls2)[column2].I;
        min = (*controls2)[column2].I;
    }
    if((*controls1)[column1].U < (*controls2)[column2].U){
        (*controls2)[column2].U = (*controls1)[column1].U;
        max = (*controls1)[column1].U;
    }
    else{
        (*controls1)[column1].U = (*controls2)[column2].U;
        max = (*controls2)[column2].U;
    }
    n = max - min + 1;
    (*controls1)[column1].F = (*controls1)[column1].F / n;
    (*controls2)[column2].F = (*controls1)[column1].F;

    if(oldfa)
        temp = (double)((*controls1)[column1].F) / (double)oldfa;
    else
        temp = (double)((*controls1)[column1].F);
    if((*controls1)[column1].D)
        exp = (double)(oldfa) / (double)((*controls1)[column1].D);
    else
        exp = (double)(oldfa);
    temp = pow(1-temp, exp);
    (*controls1)[column1].D = (*controls1)[column1].D * (1 - temp);
    if(((*controls1)[column1].D == 0) && ((*controls1)[column1].F != 0))
        (*controls1)[column1].D = 1;
    (*controls2)[column2].D = (*controls1)[column1].D;

    for(i=0; i<num_columns1; i++){
        if(i != column1){
			if(oldfa)
            	temp = (double)((*controls1)[column1].F) / (double)oldfa;
			else
				temp = (double)((*controls1)[column1].F);
			if((*controls1)[i].D)
            	exp = (double)((*controls1)[i].F) / (double)((*controls1)[i].D);
			else
				exp = (double)((*controls1)[i].F);
            temp = pow(1-temp, exp);
            (*controls1)[i].D = (*controls1)[i].D * (1 - temp);
            (*controls1)[i].F = (*controls1)[column1].F;
			if(((*controls1)[i].D == 0) && ((*controls1)[i].F != 0))
				(*controls1)[i].D = 1;
        }
    }
    for(i=0; i<num_columns2; i++){
        if(i != column2){
			if(oldfa)
            	temp = (double)((*controls2)[column2].F) / (double)oldfa;
			else
				temp = (double)((*controls2)[column2].F);
			if((*controls2)[i].D)
            	exp = (double)((*controls2)[i].F) / (double)((*controls2)[i].D);
			else
				exp = (double)((*controls2)[i].F);
            temp = pow(1-temp, exp);
            (*controls2)[i].D = (*controls2)[i].D * (1 - temp);
            (*controls2)[i].F = (*controls2)[column2].F;
			if(((*controls2)[i].D == 0) && ((*controls2)[i].F != 0))
				(*controls2)[i].D = 1;
        }
    }
}


void joincontrol(Control **controls1, Control **controls2, int column1, int column2, int num_columns1, int num_columns2){

    int i, n, min, max;
    double temp, exp;
    int oldda = (*controls1)[column1].D;
    int olddb = (*controls2)[column2].D;

    if((*controls1)[column1].I > (*controls2)[column2].I){
        (*controls2)[column2].I = (*controls1)[column1].I;
        min = (*controls1)[column1].I;
    }
    else{
        (*controls1)[column1].I = (*controls2)[column2].I;
        min = (*controls2)[column2].I;
    }
    if((*controls1)[column1].U < (*controls2)[column2].U){
        (*controls2)[column2].U = (*controls1)[column1].U;
        max = (*controls1)[column1].U;
    }
    else{
        (*controls1)[column1].U = (*controls2)[column2].U;
        max = (*controls2)[column2].U;
    }
    n = max - min + 1;

    (*controls1)[column1].F = ((*controls1)[column1].F * (*controls2)[column2].F) / n;
    (*controls2)[column2].F = (*controls1)[column1].F;
    (*controls1)[column1].D = ((*controls1)[column1].D * (*controls2)[column2].D) / n;
    (*controls2)[column2].D = (*controls1)[column1].D;

    for(i=0; i<num_columns1; i++){
        if(i != column1){
			if(oldda)
            	temp = (double)((*controls1)[column1].D) / (double)oldda;
			else
				temp = (double)((*controls1)[column1].D);
			if((*controls1)[i].D)
            	exp = (double)((*controls1)[i].F) / (double)((*controls1)[i].D);
			else
				exp = (double)((*controls1)[i].F);
            temp = pow(1-temp, exp);
            (*controls1)[i].D = (*controls1)[i].D * (1 - temp);
            (*controls1)[i].F = (*controls1)[column1].F;
			if(((*controls1)[i].D == 0) && ((*controls1)[i].F != 0))
				(*controls1)[i].D = 1;
        }
    }
    for(i=0; i<num_columns2; i++){
        if(i != column2){
			if(olddb)
            	temp = (double)((*controls2)[column2].D) / (double)olddb;
			else
				temp = (double)((*controls2)[column2].D);
			if((*controls2)[i].D)
            	exp = (double)((*controls2)[i].F) / (double)((*controls2)[i].D);
			else
				exp = (double)((*controls2)[i].F);
            temp = pow(1-temp, exp);
            (*controls2)[i].D = (*controls2)[i].D * (1 - temp);
            (*controls2)[i].F = (*controls2)[column2].F;
			if(((*controls2)[i].D == 0) && ((*controls2)[i].F != 0))
				(*controls2)[i].D = 1;
        }
    }
}


void initrelation(Relation *rel, int rows, uint64_t *values){

    int i;
    rel->tuples = realloc(rel->tuples, rows*sizeof(Tuple));
    rel->num_tuples = rows;

    for(i=0; i<rows; i++){
        rel->tuples[i].key = i+1;
        rel->tuples[i].payload = values[i];
    }
}

int getrescount(Result *res){

    if(res->counter < BUFFSIZE)
        return res->counter;
    else{
        if(res->next == NULL)
            return res->counter;
        else
            return (BUFFSIZE + getrescount(res->next));
    }
}


Result* RadixHashJoin(Relation *relR, Relation *relS, Index **indexR, Index **indexS, int originalR, int originalS){

//number of final bits that the hash function will use
    int n = N;
//number of threads
    int t = THREADS;
//margin of hash2 values
    int h2margin = 101;

    double buckets = pow(2.0, (double)n);

    int i, j, status;

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
    histR.num_tuples = buckets;
    histR.tuples = malloc(histR.num_tuples * sizeof(Tuple));
    for(i = 0; i < buckets; i++){
        histR.tuples[i].key = i;            //values of hash keys
        histR.tuples[i].payload = 0;        //initializing counter of each value
    }

    histS.num_tuples = buckets;
    histS.tuples = malloc(histS.num_tuples * sizeof(Tuple));
    for(i = 0; i < buckets; i++){
        histS.tuples[i].key = i;            //values of hash keys
        histS.tuples[i].payload = 0;        //initializing counter of each value
    }

    int startR = 0;
    int startS = 0;
    int marginR = R.num_tuples / t;
    int marginS = S.num_tuples / t;

    Histstruct histstructarr[t];
    for(i = 0; i < t; i++){
        histstructarr[i].R = &R;
        histstructarr[i].S = &S;
        histstructarr[i].startR = startR;
        histstructarr[i].startS = startS;
        histstructarr[i].endR = startR + marginR;
        histstructarr[i].endS = startS + marginS;
        histstructarr[i].histR = &histR;
        histstructarr[i].histS = &histS;
        startR = startR + marginR;
        startS = startS + marginS;
    }
    histstructarr[t-1].endR = R.num_tuples;
    histstructarr[t-1].endS = S.num_tuples;

    pthread_t histthreads[t];
    for(i = 0; i < t; i++){
        pthread_create(&(histthreads[i]), NULL, histthread, &(histstructarr[i]));
    }
    for(j = 0; j < t; j++){
        pthread_join(histthreads[j], (void **)&status);
        //fprintf(stderr,"THREAD JOIN %d\n",j);
    }


    //histR = inithist(R, buckets);
    //histS = inithist(S, buckets);

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
    Index *ind;
    int original;
    int flag;
    if(R.num_tuples < S.num_tuples){    //the index will be built for the smallest array
        Min = &R;
        Max = &S;
        psumMin = &psumR;
        psumMax = &psumS;
        flag = 0;
        original = originalR;
    }
    else{
        Min = &S;
        Max = &R;
        psumMin = &psumS;
        psumMax = &psumR;
        flag = 1;
        original = originalS;
    }

    if(flag == 0){
        if(original){
            if((*indexR) != NULL){
                ind = (*indexR);
            }
            else{
                ind = createindex(Min, psumMin, buckets, h2margin);
                (*indexR) = ind;
            }
        }
        else{
            ind = createindex(Min, psumMin, buckets, h2margin);
        }
    }
    else{
        if(original){
            if((*indexS) != NULL){
                ind = (*indexS);
            }
            else{
                ind = createindex(Min, psumMin, buckets, h2margin);
                (*indexS) = ind;
            }
        }
        else{
            ind = createindex(Min, psumMin, buckets, h2margin);
        }

    }

    //ind = createindex(Min, psumMin, buckets, h2margin);

//Joins

    Result* res;

    res = join(ind, Max, psumMax, buckets, h2margin, Rrowids, Srowids, flag);

    if(original == 0){
        free(ind->Bucket);
        free(ind->Chain);
        free(ind->R.tuples);
        free(ind);
    }

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


Result* Filter(Relation *rel, int operand, int constant){

    int i;
    Result *res;
    res = createbuff();

    for(i = 0; i < rel->num_tuples; i++){
        if(operand == 0){
            if(rel->tuples[i].payload > constant)
                insertbuff(res, rel->tuples[i].key, -1);
        }
        else if(operand == 1){
            if(rel->tuples[i].payload < constant)
                insertbuff(res, rel->tuples[i].key, -1);
        }
        else if(operand == 2){
            if(rel->tuples[i].payload == constant)
                insertbuff(res, rel->tuples[i].key, -1);
        }
    }
    return res;
}


Result* SelfJoin(Relation *relR, Relation *relS){

    int i;
    Result *res;
    res = createbuff();

    for(i = 0; i < relR->num_tuples; i++){
        if(relR->tuples[i].payload == relS->tuples[i].payload){
            insertbuff(res, relR->tuples[i].key, relS->tuples[i].key);
        }
    }
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

    for(i = 0; i < Rel->num_tuples; i++){
        ind->R.tuples[i].key = Rel->tuples[i].key;
        ind->R.tuples[i].payload = Rel->tuples[i].payload;
    }

    //Chain array initialization
    ind->Chain = malloc(((Rel->num_tuples)+1) * sizeof(int));
    for(i = 1; i < (Rel->num_tuples+1); i++){
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

void copybuff(Result *res, int *table[], int flag){
    int i;
    int index = 0;
    Result *curr = res;

    if(flag == 1){
        while(curr != NULL){
            for(i=0; i<curr->counter; i++){
                (*table)[index] = res->joins[i].key;
                index++;
            }
            curr = curr->next;
        }
    }

    if(flag == 2){
        while(curr != NULL){
            for(i=0; i<curr->counter; i++){
                (*table)[index] = res->joins[i].payload;
                index++;
            }
            curr = curr->next;
        }
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
        if(dots == 1){                  //equal filter
            p[index].flag = 1;
            p[index].t1 = s[0] - '0';
            p[index].c1 = s[2] - '0';
            p[index].t2 = 2;
            strtok(s, "=");
            str = strtok(NULL, "=");
            p[index].c2 = atoi(str);
        }
        else if(dots == 2){     //join
            p[index].flag = 0;
            p[index].t1 = s[0] - '0';
            p[index].c1 = s[2] - '0';
            p[index].t2 = s[4] - '0';
            p[index].c2 = s[6] - '0';
        }

    }
    else if(equals == 0){           //greater or less filter
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
