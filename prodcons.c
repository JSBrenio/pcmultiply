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


/**
 * Global variables for the producer-consumer buffer management
 * @authors Jeremiah Brenio, Luke Chung
 * AI was used to document code.
 */

/**
 * @brief Mutex lock for synchronizing access to shared resources.
 * This mutex is used to protect critical sections in the producer-consumer pattern.
 */
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief Condition variable to signal when the buffer is full.
 * Producers wait on this condition when the buffer has no space available.
 */
pthread_cond_t full = PTHREAD_COND_INITIALIZER;

/**
 * @brief Condition variable to signal when the buffer is empty.
 * Consumers wait on this condition when there are no items in the buffer to consume.
 */
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;

/** Position where producer will put next item */
int fill = 0;
/** Position where consumer will get next item */
int use = 0;
/** State counter of items currently in buffer */
int count = 0;
/** State variable matrix_count Number of matrices processed */
int matrix_count = 0;
/** State Flag indicating completion status (0: not done, numwork: done) */
int done = 0;


/**
 * @brief Adds a matrix to the bounded buffer
 * 
 * Places a matrix pointer in the buffer at the current fill position,
 * updates fill index with wrap-around, and increments counters.
 * 
 * @param value Pointer to the Matrix to be added to the buffer
 * @return EXIT_SUCCESS on successful addition
 */
int put(Matrix * value)
{
  bigmatrix[fill] = value;
  fill = (fill + 1) % BOUNDED_BUFFER_SIZE;
  count++;
  matrix_count++;
  return EXIT_SUCCESS;
}

/**
 * @brief Retrieves a matrix from the bounded buffer
 * 
 * Gets a matrix pointer from the buffer at the current use position,
 * updates use index with wrap-around, and decrements count.
 * Returns NULL if the buffer is empty.
 * 
 * @return Pointer to the retrieved Matrix, or NULL if buffer is empty
 */
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

/**
 * Matrix CONSUMER worker thread
 * Retrieves matrices from the buffer, finds compatible pairs for multiplication,
 * performs the multiplication, and displays the results.
 * 
 * @param arg Thread arguments (unused)
 * @return Pointer to ProdConsStats containing consumer thread statistics
 */
void *cons_worker(void *arg)
{
  // Initialize statistics tracking
  ProdConsStats *conStats = (ProdConsStats *)malloc(sizeof(ProdConsStats));
  conStats->matrixtotal = 0;
  conStats->multtotal = 0;
  conStats->sumtotal = 0;
  
  // Matrix pointers for multiplication operations
  Matrix *m1 = NULL;  // First matrix in multiplication
  Matrix *m2 = NULL;  // Second matrix in multiplication
  Matrix *m3 = NULL;  // Result matrix
  
  // Main processing loop
  while (1) {
    pthread_mutex_lock(&lock);
    
    // Check if we're done (buffer empty and all producers finished)
    if (count <= 0 && done >= numw) {
      pthread_cond_signal(&full);  // Wake up any waiting consumers
      pthread_mutex_unlock(&lock);
      break;
    }
    
    // Wait for matrix if buffer is empty
    while (count <= 0) {
      // Check again if we're done while waiting
      if (done >= numw) {
        pthread_cond_signal(&full);
        pthread_mutex_unlock(&lock);
        return conStats;
      }
      pthread_cond_wait(&full, &lock);
    }
    
    // Get first matrix for multiplication
    m1 = get();
    if (m1 == NULL) {
      pthread_mutex_unlock(&lock);
      continue;
    }
    
    // Update statistics
    conStats->sumtotal += SumMatrix(m1);
    conStats->matrixtotal++;
    pthread_cond_signal(&empty);  // Signal space is available
    
    // Find a compatible matrix for multiplication
    m3 = NULL;  // Reset result matrix
    while (m3 == NULL) {
      // Check if we're done while searching for compatible matrix
      if (count <= 0 && done >= numw) {
        break;
      }
      
      // Free previous second matrix if it exists
      if (m2 != NULL) {
        FreeMatrix(m2);
        m2 = NULL;
      }
      
      // Wait for more matrices if buffer is empty
      while (count <= 0 && done != numw) {
        pthread_cond_wait(&full, &lock);
      }
      
      // Get second matrix for multiplication
      m2 = get();
      if (m2 == NULL) {
        continue;  // Try again if we couldn't get a matrix
      }
      
      // Update statistics
      conStats->sumtotal += SumMatrix(m2);
      conStats->matrixtotal++;
      pthread_cond_signal(&empty);  // Signal space is available
      
      // Try to multiply matrices
      m3 = MatrixMultiply(m1, m2);
      // If m3 is NULL, matrices weren't compatible - loop will continue
    }
    
    // If we found compatible matrices, perform output
    if (m3 != NULL) {
      conStats->multtotal++;
      
      // Display the multiplication
      DisplayMatrix(m1, stdout);
      printf("    X\n");
      DisplayMatrix(m2, stdout);
      printf("    =\n");
      DisplayMatrix(m3, stdout);
      printf("\n");
      fflush(NULL);
    }
    
    // Clean up matrices
    if (m1 != NULL) FreeMatrix(m1);
    if (m2 != NULL) FreeMatrix(m2);
    if (m3 != NULL) FreeMatrix(m3);
    m1 = m2 = m3 = NULL;
    
    pthread_mutex_unlock(&lock);
  }
  
  // Final signals to avoid deadlocks
  pthread_cond_signal(&full);
  pthread_cond_signal(&empty);
  
  return conStats;
}
