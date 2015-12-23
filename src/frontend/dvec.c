#include "ngspice/ngspice.h"
#include "ngspice/dvec.h"


struct dvec *
dvec_alloc(void)
{
    struct dvec *rv = TMALLOC(struct dvec, 1);
    ZERO(rv, struct dvec);

    return rv;
}
