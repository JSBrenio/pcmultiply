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
 * @file prodcons.c
 * @author Jeremiah Brenio, Luke Chung
 * @date Feb 2025
 * @note AI was used to help document code.
 */

/**
 * Global variables for the producer-consumer buffer management
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
  bigmatrix[fill] = value;                  // Store the matrix pointer at the current fill position in the buffer
  fill = (fill + 1) % BOUNDED_BUFFER_SIZE;  // Advance fill index with wrap-around when reaching buffer end
  count++;                                  // Increment the count of items currently in the buffer
  matrix_count++;                           // Increment the total count of matrices processed so far
  return EXIT_SUCCESS;                      // Return success code indicating proper insertion
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
    return NULL;     // Return NULL if there's nothing to retrieve
  }
  Matrix *matrix = bigmatrix[use];       // Get the matrix at the current use position
  use = (use + 1) % BOUNDED_BUFFER_SIZE; // Advance use index with wrap-around
  count--;                               // Decrement the count of items in buffer
  return matrix;                         // Return the retrieved matrix pointer
}

/**
 * Matrix PRODUCER worker thread
 * Generates random matrices, calculates their element sum,
 * and places them into the shared buffer for consumers.
 * Continues until the required number of matrices have been produced.
 * 
 * @param arg Thread arguments (unused)
 * @return Pointer to ProdConsStats containing producer thread statistics
 */
void *prod_worker(void *arg)
{
  // Initialize statistics tracking structure for this producer
  ProdConsStats *prodStats = (ProdConsStats *)malloc(sizeof(ProdConsStats));
  prodStats->matrixtotal = 0;
  prodStats->multtotal = 0;
  prodStats->sumtotal = 0;
  
  // Main production loop - continues until required number of matrices are produced
  while(1) {
    // Acquire mutex lock to safely access shared buffer
    pthread_mutex_lock(&lock);
    
    // Check if we've reached the target number of matrices
    if (matrix_count >= NUMBER_OF_MATRICES) {
      pthread_cond_signal(&empty);  // Signal any waiting producers
      pthread_mutex_unlock(&lock);  // Release lock before exiting
      break;
    }
    
    // Wait while buffer is full - producers must wait for consumers to free space
    while(count == BOUNDED_BUFFER_SIZE) {
      pthread_cond_wait(&empty, &lock);
    }
    
    // Create and add a new matrix to the buffer if we haven't reached the limit
    if (matrix_count < NUMBER_OF_MATRICES) {
      Matrix *m = GenMatrixRandom();  // Generate a random matrix
      prodStats->sumtotal += SumMatrix(m);  // Update sum statistics
      put(m);  // Add matrix to the shared buffer
      prodStats->matrixtotal++;  // Increment count of matrices produced
      pthread_cond_signal(&full);  // Signal consumers that data is available
    }
    
    // Release mutex lock
    pthread_mutex_unlock(&lock);
  }
  
  // Final cleanup - mark this producer as done and notify consumers
  pthread_mutex_lock(&lock);
  done++;  // Increment count of finished producers
  pthread_cond_signal(&full);  // Signal consumers to check for completion
  pthread_mutex_unlock(&lock);
  
  return prodStats; // Return statistics about work done by this producer
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
  Matrix *m1 = NULL, *m2 = NULL, *m3 = NULL;
  
  // Main processing loop
  while (1) {
    pthread_mutex_lock(&lock);
    
    // Check if we're done (buffer empty and all producers finished)
    if (count <= 0 && done >= numw) {
      pthread_cond_signal(&full);  // Wake up any waiting consumers before unlocking
      pthread_mutex_unlock(&lock); // Release the mutex lock before breaking
      break;
    }
    
    // Wait for matrix if buffer is empty
    while (count <= 0) {
      // Check again if we're done while waiting
      if (done >= numw) {              // Check if all producer threads have finished
        pthread_cond_signal(&full);    // Signal any waiting consumer threads to check completion status
        pthread_mutex_unlock(&lock);   // Release the mutex lock before returning
        return conStats;               // Return consumer statistics and exit the thread
      }
      pthread_cond_wait(&full, &lock); // Wait for producers to add matrices to buffer (releases lock while waiting)
    }
    
    // Get first matrix for multiplication
    m1 = get();
    if (m1 == NULL) {
      pthread_mutex_unlock(&lock);
      continue; // try again if we fail to get a matrix
    }
    
    // Update statistics
    conStats->sumtotal += SumMatrix(m1);
    conStats->matrixtotal++;
    pthread_cond_signal(&empty);  // Signal space is available
    
    // Find a compatible matrix for multiplication
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

    // reset matrices again for next calculation
    m1 = m2 = m3 = NULL;
    
    pthread_mutex_unlock(&lock); // unlock after critical section
  }
  return conStats; // Return statistics about work done by this consumer
}
