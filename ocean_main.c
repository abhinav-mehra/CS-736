#include <stdio.h>
#include <stdlib.h>

#include "hwtimer.h"

/* Implement this function in serial_ocean and omp_ocean */
extern void ocean (int** grid, int xdim, int ydim, int timesteps);

void printGrid(int** grid, int xdim, int ydim);

int main(int argc, char* argv[])
{
    int xdim,ydim,timesteps;
    int** grid;
    int i,j,t;

    hwtimer_t timer;
    initTimer(&timer);
    
    /********************Get the arguments correctly (start) **************************/
    /* 
    Three input Arguments to the program
    1. X Dimension of the grid
    2. Y dimension of the grid
    3. number of timesteps the algorithm is to be performed
    */
    
    if (argc!=4) {
        printf("The Arguments you entered are wrong.\n");
        printf("./serial_ocean <x-dim> <y-dim> <timesteps>\n");
        return EXIT_FAILURE;
    } else {
        xdim = atoi(argv[1]);
        ydim = atoi(argv[2]);
        timesteps = atoi(argv[3]);
    }
    ///////////////////////Get the arguments correctly (end) //////////////////////////


    /*********************create the grid as required (start) ************************/
    /*
    The grid needs to be allocated as per the input arguments and randomly initialized.
    Remember during allocation that we want to gaurentee a contiguous block, hence the
    nasty pointer math.

    To test your code for correctness please comment this section of random initialization.
    */
    grid = (int**) malloc(ydim*sizeof(int*));
    int *temp = (int*) malloc(xdim*ydim*sizeof(int));
    for (i=0; i<ydim; i++) {
        grid[i] = &temp[i*xdim];
    }
    for (i=0; i<ydim; i++) {
        for (j=0; j<xdim; j++) {
            grid[i][j] = rand();
        }
    }
    ///////////////////////create the grid as required (end) //////////////////////////

    startTimer(&timer); // Start the time measurment here before the algorithm starts

    ocean(grid, xdim, ydim, timesteps);

    stopTimer(&timer); // End the time measuremnt here since the algorithm ended

    //Do the time calcuclation
    printf("Total Execution time: %lld ns\n", getTimerNs(&timer));

    // Free the memory we allocated for grid
    free(temp);
    free(grid);

    return EXIT_SUCCESS;
}
