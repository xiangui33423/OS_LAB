#include "../my_vm.h"
#include <time.h>
#include <pthread.h>
#define num_threads 15
#define SIZE 10

void *pointers[num_threads];
int ids[num_threads];
pthread_t threads[num_threads];
int alloc_size = 5000;

void mat_mult(void *mat1, void *mat2, int size, void *answer)
{
	int i, k, j, num1, num2, total;
	unsigned int addr_mat1, addr_mat2, addr_ans;

	if (!mat1 || !mat2 || !answer || size <= 0)
		return;

	for (i = 0; i < size; i++) {
		for (j = 0; j < size; j++) {
			total = 0;
			/* answer[i][j] += mat1[i][k] * mat2[k][j] */
			for (k = 0; k < size; k++) {
				addr_mat1 = (unsigned int) mat1 +
						(i * size * sizeof(int)) +
						(k * sizeof(int));

				addr_mat2 = (unsigned int) mat2 +
						(k * size * sizeof(int)) +
						(j * sizeof(int));

				myRead((void *) addr_mat1, &num1, sizeof(int));
				myRead((void *) addr_mat2, &num2, sizeof(int));
				total += num1 * num2;
			}
			addr_ans = (unsigned int) answer +
					(i * size * sizeof(int)) + (j * sizeof(int));
			myWrite((void *) addr_ans, &total, sizeof(int));
		}
	}
}

void *alloc_mem(void *id_arg) {
    int id = *((int *)id_arg);
    pointers[id] = myMalloc(alloc_size);
    return NULL;
}

void *put_mem(void *id_arg) {
    int val = 1;
    void *va_pointer = pointers[*((int *)id_arg)];
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            int address_a = (unsigned int)va_pointer + ((i * 5 * sizeof(int))) + (j * sizeof(int));
            myWrite((void *)address_a, &val, sizeof(int));
        }
    }
    return NULL;
}

void *mat_mem(void *id_arg) {
    int i = *((int *)id_arg);
    void *a = pointers[i];
    void *b = pointers[i + 1];
    void *c = pointers[i + 2];
    mat_mult(a, b, SIZE, c);
    return NULL;
}

void *free_mem(void *id_arg) {
    int id = *((int *)id_arg);
    myFree(pointers[id], alloc_size);
}

int main() {

    srand(time(NULL));

    for (int i = 0; i < num_threads; i++)
        ids[i] = i;

    for (int i = 0; i < num_threads; i++)
        pthread_create(&threads[i], NULL, alloc_mem, (void *)&ids[i]);

    for (int i = num_threads - 1; i >= 0; i--)
        pthread_join(threads[i], NULL);

    printf("Allocated Pointers: \n");

    for (int i = 0; i < num_threads; i++)
        printf("%x ", (int)pointers[i]);
    printf("\n");

    printf("Initializing some of the memory by in multiple threads\n");

    for (int i = 0; i < num_threads; i++)
        pthread_create(&threads[i], NULL, put_mem, (void *)&ids[i]);

    for (int i = num_threads - 1; i >= 0; i--)
        pthread_join(threads[i], NULL);

    printf("Randomly checking a thread allocation to see if everything worked correctly!\n");

    int rand_id = rand() % num_threads;
    void *a = pointers[rand_id];
    int val = 0;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            int address_a = (unsigned int)a + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            myRead((void *)address_a, &val, sizeof(int));
            printf("%d ", val);
        }
        printf("\n");
    }

    printf("Performing matrix multiplications in multiple threads threads!\n");

    for (int i = 0; i < num_threads; i+=3)
        pthread_create(&threads[i], NULL, mat_mem, (void *)&ids[i]);
    for (int i = 0; i < num_threads; i+=3)
        pthread_join(threads[i], NULL);

    printf("Randomly checking a thread allocation to see if everything worked correctly!\n");

    rand_id = (((rand() % (num_threads/3)) + 1) * 3) - 1;
    a = pointers[rand_id];
    val = 0;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            int address_a = (unsigned int)a + ((i * SIZE * sizeof(int))) + (j * sizeof(int));
            myRead((void *)address_a, &val, sizeof(int));
            printf("%d ", val);
        }
        printf("\n");
    }

    int old = (int)pointers[0];

    printf("Freeing everything in multiple threads!\n");

    for (int i = 0; i < num_threads; i++)
        pthread_create(&threads[i], NULL, free_mem, (void *)&ids[i]);

    for (int i = num_threads - 1; i >= 0; i--)
        pthread_join(threads[i], NULL);

    void *temp = (void *)1;
    int flag = 0;

    while (temp != NULL) {
        temp = myMalloc(10);
        if ((int)temp == old) {
            printf("Free Worked!\n");
            flag = 1;
            break;
        }
    }

    if (!flag) {
        printf("Some Problem with free!\n");
    }
}
