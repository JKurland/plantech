event hello {
    str s
    int i
}

request greeting -> int {
    float f
}

event template[T] {
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
