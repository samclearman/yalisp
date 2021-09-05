MAX = 18446744073709551615
# MAX = 4294967295
def parse_yl_number(yl_number):
    n = 0
    i = 0
    for x in reversed(yl_number):
        n += (x * ((MAX + 1) ** i))
        i += 1
    return n
