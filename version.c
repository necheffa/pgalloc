// Copyright (C) 2024 Alexander Necheff
// This program is licensed under the terms of the LGPLv3.
// See the COPYING and COPYING.LESSER files that came packaged with this source code for the full terms.

/*
 * Displays the version string of the currently linked pgalloc.
 * To access, include `extern char pgalloc_version[];` in your code.
 */
char pgalloc_version[] = {
// cppcheck-suppress missingInclude
#include "version.inc"
};

