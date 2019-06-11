#include "bf_decoding.h"
#include "dfr_test.h"
#include "gf2x_arith_mod_xPplusOne.h"
#include "qc_ldpc_parameters.h"

#include <string.h>

/* Tests if the current code attains the desired DFR. If that is the case,
 * computes the threshold for the second iteration of the decoder and stores
 * it in the globally accessible vector */

extern unsigned int PQCLEAN_LEDAKEMLT52_CLEAN_thresholds[2];

int PQCLEAN_LEDAKEMLT52_CLEAN_DFR_test(POSITION_T LSparse[N0][DV * M]) {

    POSITION_T LSparse_loc[N0][DV * M];

    /* Gamma matrix: an N0 x N0 block circulant matrix with block size p
     * gamma[a][b][c] stores the intersection of the first column of the a-th
     * block of L  with the c-th column of the b-th block of L */
    /* Gamma computation can be accelerated employing symmetry and QC properties */
    unsigned int gamma[N0][N0][P] = {{{0}}};
    unsigned int rotated_column[DV * M];

    unsigned int firstidx, secondidx, intersectionval;

    unsigned int gammaHist[N0][DV * M + 1] = {{0}};

    unsigned int maxMut[N0], maxMutMinusOne[N0];
    unsigned int allBlockMaxSumst, allBlockMaxSumstMinusOne;

    unsigned int toAdd, histIdx;

    /*transpose blocks of L, we need its columns */
    for (int i = 0; i < N0; i++) {
        for (int j = 0; j < DV * M; j++) {
            if (LSparse[i][j] != 0) {
                LSparse_loc[i][j] = (P - LSparse[i][j]);
            }
        }
        quicksort_sparse(LSparse_loc[i]);
    }

    for (int i = 0; i < N0; i++ ) {
        for (int j = 0; j < N0; j++ ) {
            for (int k = 0; k < P; k++) {
                /* compute the rotated sparse column needed */
                for (int idxToRotate = 0; idxToRotate < (DV * M); idxToRotate++) {
                    rotated_column[idxToRotate] = (LSparse_loc[j][idxToRotate] + k) % P;
                }
                quicksort_sparse(rotated_column);
                /* compute the intersection amount */
                firstidx = 0, secondidx = 0;
                intersectionval = 0;
                while ( (firstidx < DV * M) && (secondidx < DV * M) ) {
                    if ( LSparse_loc[i][firstidx] == rotated_column[secondidx] ) {
                        intersectionval++;
                        firstidx++;
                        secondidx++;
                    } else if ( LSparse_loc[i][firstidx] > rotated_column[secondidx] ) {
                        secondidx++;
                    } else { /*if ( LSparse_loc[i][firstidx] < rotated_column[secondidx] ) */
                        firstidx++;
                    }
                }
                gamma[i][j][k] = intersectionval;

            }
        }
    }
    for (int i = 0; i < N0; i++ ) {
        for (int j = 0; j < N0; j++ ) {
            gamma[i][j][0] = 0;
        }
    }
    /* build histogram of values in gamma */
    for (int i = 0; i < N0; i++ ) {
        for (int j = 0; j < N0; j++ ) {
            for (int k = 0; k < P; k++) {
                gammaHist[i][gamma[i][j][k]]++;
            }
        }
    }


    for (int gammaBlockRowIdx = 0; gammaBlockRowIdx < N0; gammaBlockRowIdx++) {
        toAdd = T_BAR - 1;
        maxMutMinusOne[gammaBlockRowIdx] = 0;
        histIdx = DV * M;
        while ( (histIdx > 0) && (toAdd > 0)) {
            if (gammaHist[gammaBlockRowIdx][histIdx] > toAdd ) {
                maxMutMinusOne[gammaBlockRowIdx] += histIdx * toAdd;
                toAdd = 0;
            } else {
                maxMutMinusOne[gammaBlockRowIdx] += histIdx * gammaHist[gammaBlockRowIdx][histIdx];
                toAdd -= gammaHist[gammaBlockRowIdx][histIdx];
                histIdx--;
            }
        }
        maxMut[gammaBlockRowIdx] = histIdx + maxMutMinusOne[gammaBlockRowIdx];
    }


    /*seek max values across all gamma blocks */
    allBlockMaxSumst = maxMut[0];
    allBlockMaxSumstMinusOne = maxMutMinusOne[0];
    for (int gammaBlockRowIdx = 0; gammaBlockRowIdx < N0 ; gammaBlockRowIdx++) {
        allBlockMaxSumst = allBlockMaxSumst < maxMut[gammaBlockRowIdx] ?
                           maxMut[gammaBlockRowIdx] :
                           allBlockMaxSumst;
        allBlockMaxSumstMinusOne = allBlockMaxSumstMinusOne < maxMutMinusOne[gammaBlockRowIdx] ?
                                   maxMutMinusOne[gammaBlockRowIdx] :
                                   allBlockMaxSumstMinusOne;
    }
    if (DV * M > (allBlockMaxSumstMinusOne + allBlockMaxSumst)) {
        PQCLEAN_LEDAKEMLT52_CLEAN_thresholds[1] = allBlockMaxSumst + 1;
        return 1;
    }
    return 0;
}