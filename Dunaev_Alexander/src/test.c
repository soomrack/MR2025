
#include <stdio.h>

//TODO добавить reverse строки перед for
// и убрать лишние символы перед выводом строки

void print_dots(const int number){
    char str[20];
    sprintf(str,"%d", number);
    for(int j=20;j>=0;--j){
        if(str[j]=='\0'){continue;}
        putchar(str[j]);
        if(j%3==0){
            putchar('.');
        }
    } 
    putchar('\n');
    printf("%d\n",(sizeof(str)/sizeof(str[0])));
    printf("%s\n",str);

}

int main(void){


    print_dots(100000000);

    return 0;
}
