/* arranque del SDK de WCH de la serie elegida */
#if defined(CH58x)
#include "../../../system/CH58x/SRC/Startup/startup_CH583.S"
#elif defined(CH59x)
#include "../../../system/CH59x/SRC/Startup/startup_CH592.S"
#elif defined(CH573)
#include "../../../system/CH573/SRC/Startup/startup_CH573.S"
#endif
