#pragma once

#include <highscore/libhighscore.h>

G_BEGIN_DECLS

#define GEARSYSTEM_TYPE_HS_CORE (gearsystem_hs_core_get_type())

G_DECLARE_FINAL_TYPE (GearsystemHsCore, gearsystem_hs_core, GEARSYSTEM, HS_CORE, HsCore)

GType hs_get_core_type (void);

G_END_DECLS
