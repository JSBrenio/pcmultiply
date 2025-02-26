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
int done = 0;

// Bounded buffer put() get()
int put(Matrix * value)
{
  bigmatrix[fill] = value;
  fill = (fill + 1) % BOUNDED_BUFFER_SIZE;
  count++;
  matrix_count++;
  return EXIT_SUCCESS;
}

Matrix * get()
{
  if (count <= 0) {  // Check if buffer is empty
    return NULL;
  }
  Matrix *matrix = bigmatrix[use];
  use = (use + 1) % BOUNDED_BUFFER_SIZE;
  count--;
  return matrix;
}

// Matrix PRODUCER worker thread
void *prod_worker(void *arg)
{
  ProdConsStats *prodStats = (ProdConsStats *)malloc(sizeof(ProdConsStats));
  prodStats->matrixtotal = 0;
  prodStats->multtotal = 0;
  prodStats->sumtotal = 0;
  while(1) {
    pthread_mutex_lock(&lock);
    if (matrix_count >= NUMBER_OF_MATRICES) {
      pthread_cond_signal(&empty);
      pthread_mutex_unlock(&lock);
      break;
    }
    while(count == BOUNDED_BUFFER_SIZE) { // when full
      pthread_cond_wait(&empty, &lock);
    }
    if (matrix_count < NUMBER_OF_MATRICES) {
      Matrix *m = GenMatrixRandom();
      prodStats->sumtotal += SumMatrix(m);
      put(m);
      prodStats->matrixtotal++;
      pthread_cond_signal(&full);
    }
    pthread_mutex_unlock(&lock);
  }
  pthread_mutex_lock(&lock);
  done++;
  pthread_cond_signal(&full);
  pthread_mutex_unlock(&lock);
  return prodStats;
}

// Matrix CONSUMER worker thread
void *cons_worker(void *arg)
{
  ProdConsStats *conStats = (ProdConsStats *)malloc(sizeof(ProdConsStats));
  conStats->matrixtotal = 0;
  conStats->multtotal = 0;
  conStats->sumtotal = 0;
  Matrix *m1 = NULL, *m2 = NULL, *m3 = NULL;
  while (1) {
    pthread_mutex_lock(&lock);
    if (count <= 0 && done >= numw) {
      pthread_cond_signal(&full);
      pthread_mutex_unlock(&lock);
      break;
    }
    while (count <= 0) {
      if (done >= numw) { // No more matrices will be produced
          pthread_cond_signal(&full);
          pthread_mutex_unlock(&lock);
          return conStats;
      }
      pthread_cond_wait(&full, &lock);
    }
    m1 = get();
    if (m1 == NULL) {
        pthread_mutex_unlock(&lock);
        break;
    }
    conStats->sumtotal += SumMatrix(m1);
    conStats->matrixtotal++;
    pthread_cond_signal(&empty);

    while(count <= 0 && done < numw) {
      pthread_cond_wait(&full, &lock);
    }
    m2 = get();
    if (m2 == NULL) {
        FreeMatrix(m1);
        pthread_mutex_unlock(&lock);
        break;
    }
    conStats->sumtotal += SumMatrix(m2);
    conStats->matrixtotal++;
    pthread_cond_signal(&empty);
    m3 = MatrixMultiply(m1, m2);
    while (m3 == NULL) {
      if (count <= 0 && done >= numw) {
        break;
      }
      FreeMatrix(m2);
      while(count <= 0 && done != numw) {
        pthread_cond_wait(&full, &lock);
      }
      m2 = get();
      if (m2 == NULL) {
        continue;
      }
      conStats->sumtotal += SumMatrix(m2);
      conStats->matrixtotal++;
      pthread_cond_signal(&empty);
      m3 = MatrixMultiply(m1, m2);
    }
    if (m2 == NULL) {
      pthread_mutex_unlock(&lock);
      continue;
    }
    conStats->multtotal++;
    DisplayMatrix(m1,stdout);
    printf("    X\n");
    DisplayMatrix(m2,stdout);
    printf("    =\n");
    DisplayMatrix(m3,stdout);
    printf("\n");
    fflush(NULL);

    if (m1 != NULL) FreeMatrix(m1);
    if (m2 != NULL) FreeMatrix(m2);
    if (m3 != NULL) FreeMatrix(m3);
    m1 = m2 = m3 = NULL;
    pthread_mutex_unlock(&lock);
  }
  pthread_cond_signal(&full);
  pthread_cond_signal(&empty);
  return conStats;
}
