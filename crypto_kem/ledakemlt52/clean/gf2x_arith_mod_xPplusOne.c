#include "gf2x_arith_mod_xPplusOne.h"
#include "rng.h"
#include "sort.h"

#include <string.h>  // memcpy(...), memset(...)


void PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_copy(DIGIT dest[], const DIGIT in[]) {
    for (int i = NUM_DIGITS_GF2X_ELEMENT - 1; i >= 0; i--) {
        dest[i] = in[i];
    }
}

/* returns the coefficient of the x^exponent term as the LSB of a digit */
DIGIT PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_get_coeff(const DIGIT poly[], unsigned int exponent) {
    unsigned int straightIdx = (NUM_DIGITS_GF2X_ELEMENT * DIGIT_SIZE_b - 1) - exponent;
    unsigned int digitIdx = straightIdx / DIGIT_SIZE_b;
    unsigned int inDigitIdx = straightIdx % DIGIT_SIZE_b;
    return (poly[digitIdx] >> (DIGIT_SIZE_b - 1 - inDigitIdx)) & ((DIGIT) 1) ;
}

/* sets the coefficient of the x^exponent term as the LSB of a digit */
void PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_set_coeff(DIGIT poly[], unsigned int exponent, DIGIT value) {
    unsigned int straightIdx = (NUM_DIGITS_GF2X_ELEMENT * DIGIT_SIZE_b - 1) - exponent;
    unsigned int digitIdx = straightIdx / DIGIT_SIZE_b;
    unsigned int inDigitIdx = straightIdx % DIGIT_SIZE_b;

    /* clear given coefficient */
    DIGIT mask = ~( ((DIGIT) 1) << (DIGIT_SIZE_b - 1 - inDigitIdx));
    poly[digitIdx] = poly[digitIdx] & mask;
    poly[digitIdx] = poly[digitIdx] | (( value & ((DIGIT) 1)) << (DIGIT_SIZE_b - 1 - inDigitIdx));
}

/* toggles (flips) the coefficient of the x^exponent term as the LSB of a digit */
void PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_toggle_coeff(DIGIT poly[], unsigned int exponent) {
    unsigned int straightIdx = (NUM_DIGITS_GF2X_ELEMENT * DIGIT_SIZE_b - 1) - exponent;
    unsigned int digitIdx = straightIdx / DIGIT_SIZE_b;
    unsigned int inDigitIdx = straightIdx % DIGIT_SIZE_b;

    /* clear given coefficient */
    DIGIT mask = ( ((DIGIT) 1) << (DIGIT_SIZE_b - 1 - inDigitIdx));
    poly[digitIdx] = poly[digitIdx] ^ mask;
}

/* population count for an unsigned 64-bit integer
   Source: Hacker's delight, p.66  */
static int popcount_uint64t(uint64_t x) {
    x -= (x >> 1) & 0x5555555555555555;
    x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
    x = (x + (x >> 4)) & 0x0f0f0f0f0f0f0f0f;
    return (int)((x * 0x0101010101010101) >> 56);
}

/* population count for a single polynomial */
int PQCLEAN_LEDAKEMLT52_CLEAN_population_count(DIGIT *poly) {
    int ret = 0;
    for (int i = NUM_DIGITS_GF2X_ELEMENT - 1; i >= 0; i--) {
        ret += popcount_uint64t(poly[i]);
    }
    return ret;
}

void PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_mod_add(DIGIT Res[], const DIGIT A[], const DIGIT B[]) {
    PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_add(Res, A, B, NUM_DIGITS_GF2X_ELEMENT);
}

static void gf2x_mod(DIGIT out[], const DIGIT in[]) {

    int i, j, posTrailingBit, maskOffset;
    DIGIT mask, aux[2 * NUM_DIGITS_GF2X_ELEMENT];

    memcpy(aux, in, 2 * NUM_DIGITS_GF2X_ELEMENT * DIGIT_SIZE_B);
    memset(out, 0x00, NUM_DIGITS_GF2X_ELEMENT * DIGIT_SIZE_B);

    for (i = 0; i < (2 * NUM_DIGITS_GF2X_ELEMENT) - NUM_DIGITS_GF2X_MODULUS; i += 1) {
        for (j = DIGIT_SIZE_b - 1; j >= 0; j--) {
            mask = ((DIGIT)0x1) << j;
            if (aux[i] & mask) {
                aux[i] ^= mask;
                posTrailingBit = (DIGIT_SIZE_b - 1 - j) + i * DIGIT_SIZE_b + P;
                maskOffset = (DIGIT_SIZE_b - 1 - (posTrailingBit % DIGIT_SIZE_b));
                mask = (DIGIT) 0x1 << maskOffset;
                aux[posTrailingBit / DIGIT_SIZE_b] ^= mask;
            }
        }
    }

    for (j = DIGIT_SIZE_b - 1; j >= MSb_POSITION_IN_MSB_DIGIT_OF_MODULUS; j--) {
        mask = ((DIGIT)0x1) << j;
        if (aux[i] & mask) {
            aux[i] ^= mask;
            posTrailingBit = (DIGIT_SIZE_b - 1 - j) + i * DIGIT_SIZE_b + P;
            maskOffset = (DIGIT_SIZE_b - 1 - (posTrailingBit % DIGIT_SIZE_b));
            mask = (DIGIT) 0x1 << maskOffset;
            aux[posTrailingBit / DIGIT_SIZE_b] ^= mask;
        }
    }

    for (i = 0; i < NUM_DIGITS_GF2X_ELEMENT; i++) {
        out[NUM_DIGITS_GF2X_ELEMENT - 1 - i] = aux[2 * NUM_DIGITS_GF2X_ELEMENT - 1 - i];
    }

}

static void right_bit_shift(unsigned int length, DIGIT in[]) {

    unsigned int j;
    for (j = length - 1; j > 0 ; j--) {
        in[j] >>= 1;
        in[j] |=  (in[j - 1] & (DIGIT)0x01) << (DIGIT_SIZE_b - 1);
    }
    in[j] >>= 1;
}


/* shifts by whole digits */
static void left_DIGIT_shift_n(unsigned int length, DIGIT in[], unsigned int amount) {
    unsigned int j;
    for (j = 0; (j + amount) < length; j++) {
        in[j] = in[j + amount];
    }
    for (; j < length; j++) {
        in[j] = (DIGIT)0;
    }
}

/* may shift by an arbitrary amount*/
static void left_bit_shift_wide_n(const int length, DIGIT in[], unsigned int amount) {
    left_DIGIT_shift_n(length, in, amount / DIGIT_SIZE_b);
    PQCLEAN_LEDAKEMLT52_CLEAN_left_bit_shift_n(length, in, amount % DIGIT_SIZE_b);
}

/* Hackers delight, reverses a uint64_t */
static DIGIT reverse_digit(DIGIT x) {
    uint64_t t;
    x = (x << 31) | (x >> 33);
    t = (x ^ (x >> 20)) & 0x00000FFF800007FFLL;
    x = (t | (t << 20)) ^ x;
    t = (x ^ (x >> 8)) & 0x00F8000F80700807LL;
    x = (t | (t << 8)) ^ x;
    t = (x ^ (x >> 4)) & 0x0808708080807008LL;
    x = (t | (t << 4)) ^ x;
    t = (x ^ (x >> 2)) & 0x1111111111111111LL;
    x = (t | (t << 2)) ^ x;
    return x;
}

void PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_transpose_in_place(DIGIT A[]) {
    /* it keeps the lsb in the same position and
     * inverts the sequence of the remaining bits */

    DIGIT mask = (DIGIT)0x1;
    DIGIT rev1, rev2, a00;
    int i, slack_bits_amount = NUM_DIGITS_GF2X_ELEMENT * DIGIT_SIZE_b - P;

    a00 = A[NUM_DIGITS_GF2X_ELEMENT - 1] & mask;
    right_bit_shift(NUM_DIGITS_GF2X_ELEMENT, A);

    for (i = NUM_DIGITS_GF2X_ELEMENT - 1; i >= (NUM_DIGITS_GF2X_ELEMENT + 1) / 2; i--) {
        rev1 = reverse_digit(A[i]);
        rev2 = reverse_digit(A[NUM_DIGITS_GF2X_ELEMENT - 1 - i]);
        A[i] = rev2;
        A[NUM_DIGITS_GF2X_ELEMENT - 1 - i] = rev1;
    }

    // A[NUM_DIGITS_GF2X_ELEMENT / 2] = reverse_digit(A[NUM_DIGITS_GF2X_ELEMENT / 2]); // no middle digit

    if (slack_bits_amount) {
        PQCLEAN_LEDAKEMLT52_CLEAN_right_bit_shift_n(NUM_DIGITS_GF2X_ELEMENT, A, slack_bits_amount);
    }
    A[NUM_DIGITS_GF2X_ELEMENT - 1] = (A[NUM_DIGITS_GF2X_ELEMENT - 1] & (~mask)) | a00;
}

static void rotate_bit_right(DIGIT in[]) { /*  x^{-1} * in(x) mod x^P+1 */

    DIGIT rotated_bit = in[NUM_DIGITS_GF2X_ELEMENT - 1] & ((DIGIT)0x1);
    right_bit_shift(NUM_DIGITS_GF2X_ELEMENT, in);
    int msb_offset_in_digit = MSb_POSITION_IN_MSB_DIGIT_OF_MODULUS - 1;
    rotated_bit = rotated_bit << msb_offset_in_digit;
    in[0] |= rotated_bit;
}

/* cond swap: swaps digits A and B if swap_mask == -1 */
static void gf2x_cswap(DIGIT *a, DIGIT *b, int swap_mask, int length) {
    int i;
    DIGIT t;
    for (i = 0; i < length; i++) {
        t = swap_mask & (a[i] ^ b[i]);
        a[i] ^= t;
        b[i] ^= t;
    }
}


/* returns -1 mask if x != 0, otherwise 0 */
static inline int nonzero(DIGIT x) {
    uint64_t t = x;
    t = -t;
    t >>= 63;
    return -(int)t;
}

/* returns -1 mask if x < 0 else 0 */
static inline int negative(int x) {
    uint32_t u = x;
    u >>= 31;
    return -(int)u;
}

/* return f(0) as digit */
static inline DIGIT lsb(const DIGIT *p) {
    DIGIT mask = (DIGIT)1;
    return p[NUM_DIGITS_GF2X_ELEMENT - 1] & mask;
}

/* constant-time inverse, source: gcd.cr.yp.to */
int PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_mod_inverse(DIGIT out[], const DIGIT in[]) {
    int i, loop, swap, mask, delta = 1;

    DIGIT f[NUM_DIGITS_GF2X_MODULUS] = {0}; // f = x^P + 1
    DIGIT g[NUM_DIGITS_GF2X_ELEMENT];       // g = in
    DIGIT v[NUM_DIGITS_GF2X_ELEMENT] = {0}; // v = 0
    DIGIT r[NUM_DIGITS_GF2X_ELEMENT] = {0}; // r = 1

    f[NUM_DIGITS_GF2X_MODULUS - 1] = 1;
    f[0] |= ((DIGIT)1 << MSb_POSITION_IN_MSB_DIGIT_OF_MODULUS);
    for (i = 0; i < NUM_DIGITS_GF2X_ELEMENT; i++) {
        g[i] = in[i];
    }
    r[NUM_DIGITS_GF2X_ELEMENT - 1] = 1;

    for (loop = 0; loop < 2 * P - 1; ++loop) {

        swap = negative(-delta) & nonzero(lsb(g));              // swap = -1 if -delta < 0 AND g(0) != 0
        delta ^= swap & (delta ^ -delta);                       // cond swap delta with -delta if swap
        delta++;

        gf2x_cswap(f, g, swap, NUM_DIGITS_GF2X_ELEMENT);
        gf2x_cswap(v, r, swap, NUM_DIGITS_GF2X_ELEMENT);

        mask = -lsb(g);

        // g = (g - g0 * f) / x
        for (i = 0; i < NUM_DIGITS_GF2X_ELEMENT; i++) {
            g[i] = g[i] ^ (mask & f[i]);
        }
        right_bit_shift(NUM_DIGITS_GF2X_ELEMENT, g);

        // r = (r - g0 * v) / x
        for (i = 0; i < NUM_DIGITS_GF2X_ELEMENT; i++) {
            r[i] = r[i] ^ (mask & v[i]);
        }
        rotate_bit_right(r);
    }

    for (i = 0; i < NUM_DIGITS_GF2X_ELEMENT; i++) {
        out[i] = v[i];
    }

    return nonzero(delta); // -1 if fail, 0 if success
}

void PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_mod_mul(DIGIT Res[], const DIGIT A[], const DIGIT B[]) {

    DIGIT aux[2 * NUM_DIGITS_GF2X_ELEMENT];
    GF2X_MUL(2 * NUM_DIGITS_GF2X_ELEMENT, aux,
             NUM_DIGITS_GF2X_ELEMENT, A,
             NUM_DIGITS_GF2X_ELEMENT, B);
    gf2x_mod(Res, aux);

}

/*PRE: the representation of the sparse coefficients is sorted in increasing
 order of the coefficients themselves */
void PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_mod_mul_dense_to_sparse(
    DIGIT Res[],
    const DIGIT dense[],
    POSITION_T sparse[], unsigned int nPos) {

    DIGIT aux[2 * NUM_DIGITS_GF2X_ELEMENT] = {0x00};
    DIGIT resDouble[2 * NUM_DIGITS_GF2X_ELEMENT] = {0x00};
    memcpy(aux + NUM_DIGITS_GF2X_ELEMENT, dense, NUM_DIGITS_GF2X_ELEMENT * DIGIT_SIZE_B);
    memcpy(resDouble + NUM_DIGITS_GF2X_ELEMENT, dense,
           NUM_DIGITS_GF2X_ELEMENT * DIGIT_SIZE_B);

    if (sparse[0] != INVALID_POS_VALUE) {
        left_bit_shift_wide_n(2 * NUM_DIGITS_GF2X_ELEMENT, resDouble, sparse[0]);
        left_bit_shift_wide_n(2 * NUM_DIGITS_GF2X_ELEMENT, aux, sparse[0]);

        for (unsigned int i = 1; i < nPos; i++) {
            if (sparse[i] != INVALID_POS_VALUE) {
                left_bit_shift_wide_n(2 * NUM_DIGITS_GF2X_ELEMENT, aux, (sparse[i] - sparse[i - 1]) );
                PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_add(resDouble, aux, resDouble, 2 * NUM_DIGITS_GF2X_ELEMENT);
            }
        }
    }

    gf2x_mod(Res, resDouble);

}

void PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_transpose_in_place_sparse(int sizeA, POSITION_T A[]) {

    POSITION_T t;
    int i = 0, j;

    if (A[i] == 0) {
        i = 1;
    }
    j = i;

    for (; i < sizeA && A[i] != INVALID_POS_VALUE; i++) {
        A[i] = P - A[i];
    }

    for (i -= 1; j < i; j++, i--) {
        t = A[j];
        A[j] = A[i];
        A[i] = t;
    }

}

void PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_mod_mul_sparse(size_t sizeR, POSITION_T Res[],
        size_t sizeA, const POSITION_T A[],
        size_t sizeB, const POSITION_T B[]) {

    /* compute all the coefficients, filling invalid positions with P*/
    size_t lastFilledPos = 0;
    for (size_t i = 0 ; i < sizeA ; i++) {
        for (size_t j = 0 ; j < sizeB ; j++) {
            uint32_t prod = A[i] + B[j];
            prod = ( (prod >= P) ? prod - P : prod);
            if ((A[i] != INVALID_POS_VALUE) &&
                    (B[j] != INVALID_POS_VALUE)) {
                Res[lastFilledPos] = prod;
            } else {
                Res[lastFilledPos] = INVALID_POS_VALUE;
            }
            lastFilledPos++;
        }
    }
    while (lastFilledPos < sizeR) {
        Res[lastFilledPos] = INVALID_POS_VALUE;
        lastFilledPos++;
    }
    PQCLEAN_LEDAKEMLT52_CLEAN_uint32_sort(Res, DV * M);
    /* eliminate duplicates */
    POSITION_T lastReadPos = Res[0];
    int duplicateCount;
    size_t write_idx = 0;
    size_t read_idx = 0;
    while (read_idx < sizeR  && Res[read_idx] != INVALID_POS_VALUE) {
        lastReadPos = Res[read_idx];
        read_idx++;
        duplicateCount = 1;
        while ( (Res[read_idx] == lastReadPos) && (Res[read_idx] != INVALID_POS_VALUE)) {
            read_idx++;
            duplicateCount++;
        }
        if (duplicateCount % 2) {
            Res[write_idx] = lastReadPos;
            write_idx++;
        }
    }
    /* fill remaining cells with INVALID_POS_VALUE */
    for (; write_idx < sizeR; write_idx++) {
        Res[write_idx] = INVALID_POS_VALUE;
    }
}

/* the implementation is safe even in case A or B alias with the result
 * PRE: A and B should be sorted, disjunct arrays ending with INVALID_POS_VALUE */
void PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_mod_add_sparse(
    int sizeR, POSITION_T Res[],
    int sizeA, const POSITION_T A[],
    int sizeB, const POSITION_T B[]) {

    POSITION_T tmpRes[DV * M];
    int idxA = 0, idxB = 0, idxR = 0;
    while ( idxA < sizeA  &&
            idxB < sizeB  &&
            A[idxA] != INVALID_POS_VALUE &&
            B[idxB] != INVALID_POS_VALUE ) {

        if (A[idxA] == B[idxB]) {
            idxA++;
            idxB++;
        } else {
            if (A[idxA] < B[idxB]) {
                tmpRes[idxR] = A[idxA];
                idxA++;
            } else {
                tmpRes[idxR] = B[idxB];
                idxB++;
            }
            idxR++;
        }
    }

    while (idxA < sizeA && A[idxA] != INVALID_POS_VALUE) {
        tmpRes[idxR] = A[idxA];
        idxA++;
        idxR++;
    }

    while (idxB < sizeB && B[idxB] != INVALID_POS_VALUE) {
        tmpRes[idxR] = B[idxB];
        idxB++;
        idxR++;
    }

    while (idxR < sizeR) {
        tmpRes[idxR] = INVALID_POS_VALUE;
        idxR++;
    }
    memcpy(Res, tmpRes, sizeof(POSITION_T)*sizeR);

}

/* Return a uniform random value in the range 0..n-1 inclusive,
 * applying a rejection sampling strategy and exploiting as a random source
 * the NIST seedexpander seeded with the proper key.
 * Assumes that the maximum value for the range n is 2^32-1
 */
static uint32_t rand_range(const unsigned int n, const int logn, AES_XOF_struct *seed_expander_ctx) {
    unsigned long required_rnd_bytes = (logn + 7) / 8;
    unsigned char rnd_char_buffer[4];
    uint32_t rnd_value;
    uint32_t mask = ( (uint32_t)1 << logn) - 1;

    do {
        PQCLEAN_LEDAKEMLT52_CLEAN_seedexpander(seed_expander_ctx, rnd_char_buffer, required_rnd_bytes);
        /* obtain an endianness independent representation of the generated random
         bytes into an unsigned integer */
        rnd_value =  ((uint32_t)rnd_char_buffer[3] << 24) +
                     ((uint32_t)rnd_char_buffer[2] << 16) +
                     ((uint32_t)rnd_char_buffer[1] <<  8) +
                     ((uint32_t)rnd_char_buffer[0] <<  0) ;
        rnd_value = mask & rnd_value;
    } while (rnd_value >= n);

    return rnd_value;
}

/* Obtains fresh randomness and seed-expands it until all the required positions
 * for the '1's in the circulant block are obtained */
void PQCLEAN_LEDAKEMLT52_CLEAN_rand_circulant_sparse_block(POSITION_T *pos_ones,
        int countOnes,
        AES_XOF_struct *seed_expander_ctx) {

    int duplicated, placedOnes = 0;
    uint32_t p;

    while (placedOnes < countOnes) {
        p = rand_range(NUM_BITS_GF2X_ELEMENT,
                       P_BITS,
                       seed_expander_ctx);
        duplicated = 0;
        for (int j = 0; j < placedOnes; j++) {
            if (pos_ones[j] == p) {
                duplicated = 1;
            }
        }
        if (duplicated == 0) {
            pos_ones[placedOnes] = p;
            placedOnes++;
        }
    }
}

/* Returns random weight-t circulant block */
void PQCLEAN_LEDAKEMLT52_CLEAN_rand_circulant_blocks_sequence(
    DIGIT sequence[N0 * NUM_DIGITS_GF2X_ELEMENT],
    AES_XOF_struct *seed_expander_ctx) {

    int rndPos[NUM_ERRORS_T],  duplicated, counter = 0;
    int p, polyIndex, exponent;

    memset(sequence, 0x00, N0 * NUM_DIGITS_GF2X_ELEMENT * DIGIT_SIZE_B);

    while (counter < NUM_ERRORS_T) {
        p = rand_range(N0 * NUM_BITS_GF2X_ELEMENT, P_BITS,
                       seed_expander_ctx);
        duplicated = 0;
        for (int j = 0; j < counter; j++) {
            if (rndPos[j] == p) {
                duplicated = 1;
            }
        }
        if (duplicated == 0) {
            rndPos[counter] = p;
            counter++;
        }
    }
    for (int j = 0; j < counter; j++) {
        polyIndex = rndPos[j] / P;
        exponent = rndPos[j] % P;
        PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_set_coeff( sequence + NUM_DIGITS_GF2X_ELEMENT * polyIndex, exponent,
                ( (DIGIT) 1));
    }

}

void PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_tobytes(uint8_t *bytes, const DIGIT *poly) {
    size_t i, j;
    for (i = 0; i < NUM_DIGITS_GF2X_ELEMENT; i++) {
        for (j = 0; j < DIGIT_SIZE_B; j++) {
            bytes[i * DIGIT_SIZE_B + j] = (uint8_t) (poly[i] >> 8 * j);
        }
    }
}

void PQCLEAN_LEDAKEMLT52_CLEAN_gf2x_frombytes(DIGIT *poly, const uint8_t *poly_bytes) {
    size_t i, j;
    for (i = 0; i < NUM_DIGITS_GF2X_ELEMENT; i++) {
        poly[i] = (DIGIT) 0;
        for (j = 0; j < DIGIT_SIZE_B; j++) {
            poly[i] |= (DIGIT) poly_bytes[i * DIGIT_SIZE_B + j] << 8 * j;
        }
    }
}
