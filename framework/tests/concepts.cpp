#include "framework/concepts.h"

struct BadRequest {};

static_assert(pt::Request<BadRequest> == false, "");
