#include "libint_internal.h"
#include <assert.h>
#include <ctype.h>

// Skip characters from input as long as isspace(*c) == true or end of file is reached.
// If so *c sets to '\0'.
LibintError libint_skip_whitespace(Libint *libint, LibisInputStream *input, bool *eof, char *c) {
    LibintError err = LIBINT_ERROR_OK;
    if (!libint || !input || !c) {
        err = LIBINT_ERROR_BAD_ARGUMENT;
        goto end;
    }
    err = EIS(libis_lookahead(libint->libis, input, eof, 1, c));
    if (*eof || err) {
        goto end;
    }
    while (isspace(*c)) {
        err = EIS(libis_skip_char(libint->libis, input, eof, c));
        if (*eof || err) {
            goto end;
        }
    }
end:
    return err;
}

