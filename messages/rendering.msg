data RenderingBufferHandle {
    usize idx
}

data RenderingWriteBuffer {
    RenderingBufferHandle buffer
    list[byte] data
}

data RenderingResizeWindow {
    Extent2D newSize
}

union RenderingOp {
    RenderingWriteBuffer writeBuffer
    RenderingResizeWindow resizeWindow
}
