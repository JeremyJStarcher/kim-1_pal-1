#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdbool.h>

#define BYTES_PER_LINE 12
#define BUFFER_SIZE 64 * 1024
int hexToInt(char s[]);
/*
 *
 * infile
 * outfile
 * variable name
 * <start> (hex)
 * <end> (hex) 
 * */

int main(int argc, char *argv[])
{
    char fname[255];
    unsigned char tmp[255];
    unsigned char buf[BUFFER_SIZE];
    char *line;
    size_t bufidx = 0;

    sprintf(fname, "%s", argv[2]);

    printf("Reading: %s\n", argv[1]);
    printf("Writing : %s\n", fname);

    FILE *ifp = fopen(argv[1], "rb");
    FILE *ofp = fopen(argv[2], "w");

    if (ifp == NULL)
    {
        fprintf(stderr, "Failed to open file\n");
        fflush(stderr);
        exit(1);
    }

    while ((line = fgets(tmp, 266, ifp)) != NULL)
    {
        char *endptr;
        bool first_num = true;

        while (1)
        {
            long num = strtol(line, &endptr, 16);
            if (line == endptr)
                break;

            if (!first_num)
            {
                int value = (int)num;
                // printf("%d ", value);
                buf[bufidx] = value;
                bufidx++;
            }
            first_num = false;

            line = endptr;
        }
        // printf("\n");

        while (isspace((unsigned char)*endptr))
        {
            endptr++;
        }

        if (*endptr != '\0')
        {
            printf("Extra garbage at end of line\n");
            exit(1);
        }
    }

    //size_t size = fread(&buf, 1, BUFFER_SIZE, ifp);
    size_t size = bufidx;

    printf("The size is: %lu\n", size);
    int start = 0;
    int end = size;

    if (argc > 4)
    {
        printf("(range) start-end: %s %s\n", argv[4], argv[5]);

        start = hexToInt(argv[4]);
        if (argc > 5)
        {
            end = hexToInt(argv[5]);
        }
    }

    int length = end - start;

    printf("The length is: %d\n", length);
    printf("start-end: %04X %04X\n", start, end);
    printf("start-end: %d %d\n", start, end);

    fprintf(ofp, "const uint8_t %s[%d] PROGMEM = {", argv[3], length);
    int ctr = 0;
    for (size_t i = start; i < end; i++)
    {
        if (i != start)
        {
            fprintf(ofp, ", ");
        }

        if (ctr % BYTES_PER_LINE == 0)
        {
            if (false)
            {
                fprintf(ofp, "  // %04X - %04X\n",
                        ((int)i - BYTES_PER_LINE), (int)i - 1);
            } else {
                fprintf(ofp, "\n");
            }
        }
        ctr++;

        fprintf(ofp, "0x%02X", buf[i]);
    }
    fprintf(ofp, "};\n");
}

int hexToInt(char s[])
{
    int YES = 1 == 1;
    int NO = !YES;

    int hexdigit, i, inhex, n;
    i = 0;
    if (s[i] == '0')
    {
        ++i;
        if (s[i] == 'x' || s[i] == 'X')
        {
            ++i;
        }
    }

    n = 0;
    inhex = YES;
    for (; inhex == YES; ++i)
    {
        if (s[i] >= '0' && s[i] <= '9')
        {
            hexdigit = s[i] - '0';
        }
        else if (s[i] >= 'a' && s[i] <= 'f')
        {
            hexdigit = s[i] - 'a' + 10;
        }
        else if (s[i] >= 'A' && s[i] <= 'F')
        {
            hexdigit = s[i] - 'A' + 10;
        }
        else
        {
            inhex = NO;
        }

        if (inhex == YES)
        {
            n = 16 * n + hexdigit;
        }
    }

    return n;
}
