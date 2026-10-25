#pragma once

// Token pasting used by the registration macros.
//
// The arguments are already expanded when they arrive: they always come through
// an outer macro parameter (the __COUNTER__ of LO_REGISTER_*_IMPL), which is
// expanded once during that substitution. A single level of ## is therefore
// enough.
#define LO_CONCAT(left, right) left##right
