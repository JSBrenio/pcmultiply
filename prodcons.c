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
pthread_cond_t doneProducing = PTHREAD_COND_INITIALIZER;

//struct counter_t counter;
int fill = 0;
int use = 0;
int count = 0;
int matrix_count = 0;
int done = 0;
int i = 0;

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
  while(1) {
    pthread_mutex_lock(&lock);
    printf("PROD %lu LOOPING i = %d\n", pthread_self(),  matrix_count);
    fflush(NULL);
    if (matrix_count >= NUMBER_OF_MATRICES) {
      pthread_mutex_unlock(&lock);
      break;
    }

    while(count == BOUNDED_BUFFER_SIZE) { // when full
      printf("PROD SLEEPING\n");
      printf("Thread ID: %lu \n", pthread_self());
      fflush(NULL);
      pthread_cond_wait(&empty, &lock);
    }
    printf("PROD AWAKE\n");
    fflush(NULL);
    if (matrix_count < NUMBER_OF_MATRICES) {
      put(GenMatrixRandom());
      prodStats->matrixtotal++;
      pthread_cond_signal(&full);
    }
    pthread_mutex_unlock(&lock);
  }
  pthread_mutex_lock(&lock);
  done++;
  pthread_mutex_unlock(&lock);
  return prodStats;
}

// Matrix CONSUMER worker thread
void *cons_worker(void *arg)
{
  ProdConsStats *conStats = (ProdConsStats *)malloc(sizeof(ProdConsStats));
  printf("DEBUG CON START\n");
  fflush(NULL);

  Matrix *m1, *m2, *m3 = NULL;
  while (matrix_count < NUMBER_OF_MATRICES) {
    pthread_mutex_lock(&lock);
    printf("CON %lu LOOPING i = %d\n", pthread_self(),  matrix_count);
    fflush(NULL);
    if (count == 0 && done == numw) {
      pthread_mutex_unlock(&lock);
      return conStats;
    }
    while(count == 0 && done != numw) {
      pthread_cond_wait(&full, &lock);
    }
    m1 = get();
    i++;
    pthread_cond_signal(&empty);
    conStats->matrixtotal++;

    do {
      printf("I KEEP PRINTING\n");
      fflush(NULL);
      if (count == 0 && done == numw) {
        pthread_mutex_unlock(&lock);
        return conStats;
      }
      while(count == 0 && done != numw) {
        pthread_cond_wait(&full, &lock);
      }
      printf("IMBOUTAGET\n");
      fflush(NULL);
      m2 = get();
      i++;
      pthread_cond_signal(&empty);
      conStats->matrixtotal++;
      
      printf("IMBOUTAMULT\n");
      fflush(NULL);
      if (m1 != NULL && m2 != NULL) m3 = MatrixMultiply(m1, m2);
      printf("IMDONEMULTING\n");
      fflush(NULL);
      if (m3 == NULL && i == NUMBER_OF_MATRICES) {
        if (m3 != NULL) FreeMatrix(m3);
        if (m2 != NULL) FreeMatrix(m2);
        if (m1 != NULL) FreeMatrix(m1);
        pthread_mutex_unlock(&lock);
        return conStats;
      }
      printf("IMBOUTAIF\n");
      fflush(NULL);
      if (m3 == NULL) {
        if(m2 != NULL)FreeMatrix(m2);
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
    pthread_mutex_unlock(&lock);
  }
  return conStats;
}
