#include "graphics/hwpipeline.h"
#include <SDL3/SDL.h>

unsigned int HwPipeline::ColourFormat()
{
    return SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
}

unsigned int HwPipeline::DepthFormat(Device)
{
    return SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
}

HwPipeline::Pipeline HwPipeline::CreateMeshPipeline(Device, bool)
{
    return Pipeline{};
}

void HwPipeline::Release(Device, Pipeline)
{
}
