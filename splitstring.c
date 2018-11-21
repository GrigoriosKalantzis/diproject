#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct predicate{
    int flag;       //0 for join 1 for filter
    int t1;
    int c1;
    int t2;     //if join second table, if filter 0 for >, 1 for <, 2 for =
    int c2;     // if join second column, if filter constant number
};
typedef struct predicate Predicate;

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




int main(){

    char str[] = "0 2 4|0.1=1.2&1.0=2.1&0.1>3000|0.0 1.1";

    char *rel, *pred, *sum;
    int rels, preds, sums;
    char *s;
    int i;


    rel = strtok(str,"|");
    pred = strtok(NULL,"|");
    sum = strtok(NULL,"|");


    rels = charcounter(rel,' ');
    preds = charcounter(pred,'&');
    sums = charcounter(sum,' ');

    int relations[rels+1];
    relations[0] = atoi(strtok(rel," "));
    for(i=1; i<=rels; i++){
        relations[i] = atoi(strtok(NULL," "));
        printf("%d\n", relations[i]);
    }



    Predicate p[preds+1];

    s = strtok(pred,"&");
    insertpred(s, p, 0);

    for(i=1; i<=preds; i++){
        s = strtok(NULL,"&");
        insertpred(s, p, i);
    }

    for(i=0; i<=preds; i++){
        printf("%d %d %d %d %d\n", p[i].flag, p[i].t1, p[i].c1, p[i].t2, p[i].c2);
    }




    int checksums[sums+1][2];
    s = strtok(sum, " ");
    checksums[0][0] = s[0] - '0';
    checksums[0][1] = s[2] - '0';
    for(i=1; i<=sums; i++){
        s = strtok(NULL, " ");
        checksums[i][0] = s[0] - '0';
        checksums[i][1] = s[2] - '0';
    }

    for(i=0; i<=sums; i++){
       printf("%d %d\n", checksums[i][0], checksums[i][1]);
    }


    return 0;
}

