# yoinked from Wikipedia
# (hey, don't judge me)

from dataclasses import dataclass


@dataclass
class EuclidData:
    """gcd == s * a + t * b"""

    gcd: int
    s: int
    t: int


def euclid_gcd(a: int, b: int):
    prev_s, s = 1, 0
    prev_t, t = 0, 1

    while b > 0:
        q = a // b
        a, b = b, a % b
        prev_s, s = s, prev_s - q * s
        prev_t, t = t, prev_t - q * t

    return EuclidData(gcd=a, s=prev_s, t=prev_t)


def inverse(x: int, mod: int):
    data = euclid_gcd(x, mod)
    return data.s % mod


if __name__ == "__main__":
    import sys

    a, b = map(int, sys.argv[1:])
    res = euclid_gcd(a, b)
    print(f"{res.gcd} = ({res.s})*{a} + ({res.t})*{b}")
