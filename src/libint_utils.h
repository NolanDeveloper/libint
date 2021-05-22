#ifndef LIBINT_UTILS_H
#define LIBINT_UTILS_H

#include <libint.h>
#include <libis.h>

// Skip characters from input as long as isspace(*c) == true or end of file is reached.
// If so *c sets to '\0'.
LibintError libint_skip_whitespace(Libint *libint, LibisInputStream *input, bool *eof, char *c);

#endif
