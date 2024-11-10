// Copyright (C) 2024 Alexander Necheff
// This program is licensed under the terms of the LGPLv2.1.
// See the LICENSE file that came packaged with this source code for the full terms.

/*
 * Displays the version string of the currently linked pgalloc.
 * To access, include `extern char pgalloc_version[];` in your code.
 */
char pgalloc_version[] = {
#include "version.inc"
};

