#include "renderConfig.h"

namespace viscfg {
RenderConfig viscfg::StandardRenderConfig()
{
    return RenderConfig();
}
RenderConfig viscfg::SmallRenderConfig()
{
    auto cfg = RenderConfig();
    cfg.velocityGradient = {
    { 66, 135, 245 }, { 250, 250, 250 }, 0.01, 1. };

    return cfg;
}

}