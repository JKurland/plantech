import GuiHandle[ElemT]
import Button
import Gui::Handle

request AddButton -> GuiHandle[Button] {}

request AddGuiElement[T] -> GuiHandle[T] {
    option[Gui::Handle] parent
    T element
}

request UpdateGuiElement[T] -> void {
    GuiHandle[T] handle
    T newElement
}

request GetGuiElement[T] -> option[T] {
    GuiHandle[T] handle
}

event GuiElementMouseButton[ElemT] {
    MouseButton mouseEvent
    GuiHandle[ElemT] element
    ElemT::PosDataT posData
}
