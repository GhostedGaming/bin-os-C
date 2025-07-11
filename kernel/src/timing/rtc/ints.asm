global disable_ints
global enable_ints

disable_ints:
    cli
    ret

enable_ints:
    sti
    ret