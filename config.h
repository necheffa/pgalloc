#ifndef CONFIG_H
#define CONFIG_H

typedef struct OFFSETS OFFSETS;

// set to 0 for POSIX or 1 for Windows
#define NON_POSIX  0

// update to 8128 for final version
#define PAGE_SIZE   1024
#define BBLOCK_SIZE 8

// set to 0 for 64 bit or 1 for 32 bit
#define WORD_SIZE   0

#if WORD_SIZE == 0
struct OFFSETS {
    ;
};

#endif /* WORD_SIZE == 0 */

#endif /* CONFIG_H */
