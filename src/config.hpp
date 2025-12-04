#pragma once

/*here you can choose the text buffer backend*/

#define TB_BACKEND_VECTOR 1
#define TB_BACKEND_GAP    2
#define TB_BACKEND_ROPE   3

#ifndef TB_BACKEND
#define TB_BACKEND TB_BACKEND_ROPE
#endif

#if TB_BACKEND == TB_BACKEND_VECTOR
#define TB_BACKEND_NAME "vector"
#elif TB_BACKEND == TB_BACKEND_GAP
#define TB_BACKEND_NAME "gap"
#elif TB_BACKEND == TB_BACKEND_ROPE
#define TB_BACKEND_NAME "rope"
#else
#define TB_BACKEND_NAME "unknown"
#endif
