#ifndef PGALLOC_H
#define PGALLOC_H

#include "config.h"
#include <stddef.h>

void *pgalloc(size_t);
void pgfree(void *);
void pgview(void);

#endif /* PGALLOC_H */
