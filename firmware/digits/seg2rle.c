// this would be so easy to to in the origina python, but i couldn't get the image library installed with my stock osx python.
//
// Pack four 5-bit numbers in to bytes:
//
// aaaaa bbbbb ccccc ddddd   phase 0
// eeeee fffff ggggg hhhhh   phase 1
//
// 765 43210
// ---------
// ccc aaaaa
// ccd bbbbb
// ddd eeeee
// dhh fffff
// hhh ggggg


#include <stdio.h>
void main(void)
{
    int a,b,c,d;       // numbers from file
    char s[505];
    FILE *fin = fopen("rle-in.c","r");
    char * note;
    int phase = 0;
    int last_d;
    do {
        fgets(s, 500, fin);
        if(4==sscanf(s," %d, %d, %d, %d,", &a, &b, &c, &d)) {
            a = a & 0x1F;
            b = b & 0x1F;
            c = c & 0x1F;
            d = d & 0x1F;
            if(phase == 0) {
               printf("    ((%2d>>2)<<5) | %2d, ", c, a);
               printf(" ((%2d &3)<<6) | ((%2d>>4)<<5) | %2d,                    ", c, d, b);
               last_d = d; phase = 1;
            } else {
               printf("(((%2d>>1)&7)<<5) | %2d, ", last_d, a);
               printf("((%2d & 1)<<7) | ((%2d>>3)<<5) | %2d, ", last_d, d, b);
               printf("((%2d&7)<<5) | %2d,  ", d, c);
               phase = 0;
            }
            printf (" // original: ");

        } else {
            phase = 0;  //reset to zero at top
        }
        printf("%s", s);
    } while(!feof(fin));
    fclose(fin);
}