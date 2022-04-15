import GuiHandle[ElemT]
import Button
import Gui::Handle

request AddButton -> GuiHandle[Button] {}

request AddGuiElement[T] -> GuiHandle[T] {
    option[Gui::Handle] parent
    T element
}

event GuiElementMouseButton[ElemT] {
    MouseButton mouseEvent
    GuiHandle[ElemT] element
    ElemT::PosDataT posData
}

