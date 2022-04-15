event hello {
    str s
    int i
}

request greeting -> int {
    float f
}

event template1[T] {
    int i
}

event template2[T, U] {
    int i
}

event template3[T] {
    T t
}

request templateR[T] -> T {
    T t
}

event templateE[T] {
    T::Something s
}

import Imported
import ImportedTemplate[T]

event usesImported {
    Imported i
    ImportedTemplate[int] j
}

event optional {
    option[int] i
    list[str] l
}
