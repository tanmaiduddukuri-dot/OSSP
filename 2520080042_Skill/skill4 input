#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_INPUT 200
#define MAX_TOKENS 50

typedef struct
{
    char value[50];
} Token;

int tokenize(char *input, Token tokens[])
{
    int count = 0;
    int i = 0;

    while (input[i] != '\0' && count < MAX_TOKENS)
    {
        /* Skip whitespace */
        while (isspace((unsigned char)input[i]))
        {
            i++;
        }

        if (input[i] == '\0')
            break;

        /* Handle delimiters */
        if (input[i] == '|' || input[i] == '<' || input[i] == '>')
        {
            tokens[count].value[0] = input[i];
            tokens[count].value[1] = '\0';
            count++;
            i++;
            continue;
        }

        /* Store normal token */
        int j = 0;

        while (input[i] != '\0' &&
               !isspace((unsigned char)input[i]) &&
               input[i] != '|' &&
               input[i] != '<' &&
               input[i] != '>')
        {
            if (j < 49)
            {
                tokens[count].value[j++] = input[i];
            }

            i++;
        }

        tokens[count].value[j] = '\0';

        if (j > 0)
            count++;
    }

    return count;
}

int validate_tokens(Token tokens[], int count)
{
    if (count == 0)
    {
        printf("Validation: Empty command.\n");
        return 0;
    }

    for (int i = 0; i < count; i++)
    {
        /* A pipe cannot be the first or last token */
        if (strcmp(tokens[i].value, "|") == 0)
        {
            if (i == 0 || i == count - 1)
            {
                printf("Validation Error: Invalid pipe position.\n");
                return 0;
            }
        }
    }

    return 1;
}

int main()
{
    char input[MAX_INPUT];
    Token tokens[MAX_TOKENS];

    printf("Enter a command: ");
    fgets(input, sizeof(input), stdin);

    input[strcspn(input, "\n")] = '\0';

    int count = tokenize(input, tokens);

    printf("\n--- Tokenization Output ---\n");

    for (int i = 0; i < count; i++)
    {
        printf("Token %d: [%s]\n", i + 1, tokens[i].value);
    }

    printf("\nTotal tokens: %d\n", count);

    printf("\n--- Validation ---\n");

    if (validate_tokens(tokens, count))
    {
        printf("Token stream is valid.\n");
    }
    else
    {
        printf("Token stream is invalid.\n");
    }

    return 0;
}
