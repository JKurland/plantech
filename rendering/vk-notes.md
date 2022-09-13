vk notes
FramePipeline holds a set of resources and a dag of steps, steps reference 
    resources and the resources they reference defines the connections

Swapchain
get the swapchain, need to know window size in case the surface doesn't know
swapchain is resource
CreateSwapchain() -> Swapchain # the vulkan handler knows this will depend on window size
GetSwapchainImage(Swapchain) -> SwapchainImage
CreateImageViews(Swapchain) -> SwapchainImageView

