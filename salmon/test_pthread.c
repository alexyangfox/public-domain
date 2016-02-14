/* file "test_pthread.c" */

/*
 *  This file is used as a test to see whether pthreads is available and the
 *  Salmon Makefile knows how to use it.
 *
 *  This file is part of SalmonEye, an interpreter for the Salmon Programming
 *  Language.
 *
 *  Written by Chris Wilson.
 *
 *  This file is hearby placed in the public domain by its author.
 */


#include <pthread.h>


extern main(int argc, char *argv[])
  {
    pthread_mutex_t my_mutex;

    pthread_mutex_init(&my_mutex, NULL);
    pthread_mutex_lock(&my_mutex);
    pthread_mutex_unlock(&my_mutex);
    pthread_mutex_destroy(&my_mutex);
  }
