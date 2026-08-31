#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HISTORY 10
#define MAX_INPUT 100

char *history[MAX_HISTORY];
int history_count = 0;

void add_history(const char *command)
{
    if (history_count < MAX_HISTORY)
    {
        history[history_count] = malloc(strlen(command) + 1);

        if (history[history_count] == NULL)
        {
            perror("malloc");
            exit(1);
        }

        strcpy(history[history_count], command);
        history_count++;
    }
    else
    {
        free(history[0]);

        for (int i = 1; i < MAX_HISTORY; i++)
        {
            history[i - 1] = history[i];
        }

        history[MAX_HISTORY - 1] = malloc(strlen(command) + 1);

        if (history[MAX_HISTORY - 1] == NULL)
        {
            perror("malloc");
            exit(1);
        }

        strcpy(history[MAX_HISTORY - 1], command);
    }
}

void show_history()
{
    printf("\nCommand History:\n");

    for (int i = 0; i < history_count; i++)
    {
        printf("%d  %s\n", i + 1, history[i]);
    }
}

void free_history()
{
    for (int i = 0; i < history_count; i++)
    {
        free(history[i]);
    }
}

int main()
{
    char *input = malloc(MAX_INPUT);

    if (input == NULL)
    {
        perror("malloc");
        return 1;
    }

    printf("Simple Command History System\n");
    printf("Type 'history' to view commands.\n");
    printf("Type 'exit' to quit.\n\n");

    while (1)
    {
        printf("shell> ");

        if (fgets(input, MAX_INPUT, stdin) == NULL)
            break;

        input[strcspn(input, "\n")] = '\0';

        if (strcmp(input, "exit") == 0)
            break;

        if (strcmp(input, "history") == 0)
        {
            show_history();
            continue;
        }

        if (strlen(input) > 0)
        {
            add_history(input);
            printf("Command stored: %s\n", input);
        }
    }

    free(input);
    free_history();

    printf("Memory released successfully.\n");

    return 0;
}
