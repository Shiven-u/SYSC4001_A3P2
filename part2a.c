#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>


#define MAX_LINES 5
#define MAX_STR 256

typedef struct 
{
    char rubric[MAX_LINES][MAX_STR];
    char exam_student[10];
    int current_exam_index;
    int questions_marked[MAX_LINES]; 
} SharedData;

void random_sleep(float minSec, float maxSec)
{
    float r = minSec + ((float)rand() / RAND_MAX) * (maxSec - minSec);
    usleep((int)(r * 1000000));
}

int load_exam(SharedData *sh, int index) 
{
    char filename[256];
    sprintf(filename, "exams/exam%d.txt", index);

    FILE *f = fopen(filename, "r");
    if (!f) return 0;

    fgets(sh->exam_student, 10, f);
    fclose(f);

    sh->exam_student[strcspn(sh->exam_student, "\n")] = 0;

    return 1;
}

void load_rubric(SharedData *sh) 
{
    FILE *f = fopen("rubric.txt", "r");
    for (int i = 0; i < MAX_LINES; i++)
    {
        fgets(sh->rubric[i], MAX_STR, f);
    }
    fclose(f);
}

void save_rubric(SharedData *sh) 
{
    FILE *f = fopen("rubric.txt", "w");
    for (int i = 0; i < MAX_LINES; i++) 
    {
        fputs(sh->rubric[i], f);
    }
    fclose(f);
}

void TA_process(int id, SharedData *sh) 
{
    srand(getpid());

    while (1) 
    {
        printf("[TA %d] rviewing rubric...\n", id);

        for (int i = 0; i < MAX_LINES; i++) 
        {
            random_sleep(0.5, 1.0);

            if (rand() % 5 == 0) 
            {
                char *comma = strchr(sh->rubric[i], ',');
                if (comma && comma[1] != '\0') 
                {
                    char oldChar = comma[1];
                    comma[1] = comma[1] + 1; // Increment character
                    char newChar = comma[1];

                    printf("[TA %d] Corrected rubric line %d: '%c' - '%c'\n",
                        id, i + 1, oldChar, newChar);
                }
                save_rubric(sh);
            }
        }

        printf("[TA %d] Reviewing exam for student %s\n", id, sh->exam_student);

        for (int q = 0; q < MAX_LINES; q++) 
        {
            if (sh->questions_marked[q] == 0) 
            {
                sh->questions_marked[q] = 1;

                printf("[TA %d] Marking question %d student %s\n",
                       id, q + 1, sh->exam_student);

                random_sleep(1.0, 2.0);
            }
        }

        if (strcmp(sh->exam_student, "9999") == 0) 
        {
            printf("[TA %d] Student 9999 reached.\n", id);
            exit(0);
        }

        int next = sh->current_exam_index + 1;
        sh->current_exam_index = next;

        memset(sh->questions_marked, 0, sizeof(sh->questions_marked));

        if (!load_exam(sh, next)) 
        {
            printf("[TA %d] No more exams.\n", id);
            exit(0);
        }
    }
}

int main(int argc, char *argv[]) 
{
    if (argc < 2) 
    {
        printf("Usage: ./part2a <num_TAs>\n");
        return 1;
    }

    int num_TAs = atoi(argv[1]);

    int shmid = shmget(IPC_PRIVATE, sizeof(SharedData), 0666 | IPC_CREAT);
    SharedData *sh = (SharedData *)shmat(shmid, NULL, 0);

    memset(sh, 0, sizeof(SharedData));
    sh->current_exam_index = 1;

    load_rubric(sh);
    load_exam(sh, 1);

    for (int i = 0; i < num_TAs; i++) 
    {
        pid_t pid = fork();
        if (pid == 0) 
        {
            TA_process(i + 1, sh);
            exit(0);
        }
    }

    while (wait(NULL) > 0);

    shmdt(sh);
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
