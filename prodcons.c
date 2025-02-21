/*
 *  prodcons module
 *  Producer Consumer module
 *
 *  Implements routines for the producer consumer module based on
 *  chapter 30, section 2 of Operating Systems: Three Easy Pieces
 *
 *  University of Washington, Tacoma
 *  TCSS 422 - Operating Systems
 */

// Include only libraries for this module
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "counter.h"
#include "matrix.h"
#include "pcmatrix.h"
#include "prodcons.h"


// Define Locks, Condition variables, and so on here
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

//struct counter_t counter;
int fill = 0;
int use = 0;
int count = 0;
int matrix_count = 0;

// Bounded buffer put() get()
int put(Matrix * value)
{
  bigmatrix[fill] = value;
  fill = (fill + 1) % BOUNDED_BUFFER_SIZE;
  count++;
  matrix_count++;
  //increment_cnt(&counter);
  return EXIT_SUCCESS;
}

Matrix * get()
{
  Matrix *matrix = bigmatrix[use];
  use = (use + 1) % BOUNDED_BUFFER_SIZE;
  count--;
  return matrix;
}

// Matrix PRODUCER worker thread
void *prod_worker(void *arg)
{
  ProdConsStats *prodStats = (ProdConsStats *)malloc(sizeof(ProdConsStats));
  printf("DEBUG PROD START\n");
  fflush(NULL);
  for (int i = 0; i < NUMBER_OF_MATRICES; i++) {
    printf("PROD LOOPING i = %d\n", i);
    fflush(NULL);

    pthread_mutex_lock(&lock);
    while(count == BOUNDED_BUFFER_SIZE) {
      pthread_cond_wait(&empty, &lock);
    }
    put(GenMatrixRandom());
    pthread_cond_signal(&full);
    pthread_mutex_unlock(&lock);
  }
  return NULL;
}

// Matrix CONSUMER worker thread
void *cons_worker(void *arg)
{
  ProdConsStats *conStats = (ProdConsStats *)malloc(sizeof(ProdConsStats));
  printf("DEBUG CON START\n");
  fflush(NULL);

  Matrix *m1, *m2, *m3;
  for (int i=0;i<NUMBER_OF_MATRICES;i++)
  {
    printf("CON LOOPING i = %d\n", i);
    fflush(NULL);
    pthread_mutex_lock(&lock);
    while(count == 0) {
      pthread_cond_wait(&full, &lock);
    }
    m1 = get();
    pthread_cond_signal(&empty);
    conStats->matrixtotal++;

    do {
      printf("I KEEP PRINTING\n");
      fflush(NULL);
      while(count == 0 && matrix_count != NUMBER_OF_MATRICES) {
        pthread_cond_wait(&full, &lock);
      }
      m2 = get();
      pthread_cond_signal(&empty);
      conStats->matrixtotal++;

      if (m3 != NULL) {
        FreeMatrix(m2);
      }
      m3 = MatrixMultiply(m1, m2);
      if (m3 == NULL && matrix_count == NUMBER_OF_MATRICES) {
        FreeMatrix(m3);
        FreeMatrix(m2);
        FreeMatrix(m1);
        return;
      }
    } while (m3 == NULL);
    conStats->multtotal++;
    DisplayMatrix(m1,stdout);
    printf("    X\n");
    DisplayMatrix(m2,stdout);
    printf("    =\n");
    DisplayMatrix(m3,stdout);
    printf("\n");
    fflush(NULL);

    FreeMatrix(m3);
    FreeMatrix(m2);
    FreeMatrix(m1);
    if ( matrix_count == NUMBER_OF_MATRICES) {
      return;
    }
    pthread_mutex_unlock(&lock);
  }
  return NULL;
}
