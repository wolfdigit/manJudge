#include <stdio.h>

int main() {
    char ch;
    while (scanf("%c", &ch)!=EOF) {
        printf("%d\t", ch);
    }

    return 0;
}
