#include "streamx/core/buffer.h"

// Explicit template instantiation for Frame buffers
namespace streamx {
template class FrameBuffer<Frame>;
template class FrameBuffer<Packet>;
}
