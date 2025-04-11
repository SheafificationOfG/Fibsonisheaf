from dataclasses import dataclass
from functools import cache


@dataclass(frozen=True)
class Number:
    even_exp: int
    odd_part: int

    @classmethod
    def from_int(cls, x: int):
        x -= 1
        s = 0
        while x & 1 == 0:
            x >>= 1
            s += 1
        return cls(even_exp=s, odd_part=x)

    @cache
    def __int__(self):
        return (1 << self.even_exp) * self.odd_part + 1

    def is_prime(self) -> bool:
        """Miller primality test.

        Yoinked from wikipedia (hey, don't judge me)."""
        n = int(self)
        s = self.even_exp
        d = self.odd_part

        for a in range(2, 1 + min(n - 2, 2 * s * len(f"{d:b}"))):
            x = pow(a, d, n)
            for _ in range(s):
                if x in (1, n - 1):
                    x = 1
                    break
                x = pow(x, 2, n)
                if x == 1:
                    return False
            if x != 1:
                return False
        return True


def split(n: int):
    n -= 1
    s = 0
    while n & 1 == 0:
        n >>= 1
        s += 1
    return (s, n)


def miller(s: int, d: int):
    assert d & 1, "d must be odd!"
    n = (1 << s) * d + 1
    for a in range(2, 1 + min(n - 2, 2 * s * len(f"{d:b}"))):
        x = pow(a, d, n)
        for _ in range(s):
            if x in (1, n - 1):
                x = 1
                break
            x = pow(x, 2, n)
            if x == 1:
                return False
        if x != 1:
            return False
    return True


def find_candidate(bitlen: int):
    for s in range(bitlen - 1, 0, -1):
        d = 1
        while d.bit_length() + s <= bitlen:
            num = Number(s, d)
            if num.is_prime():
                return num
            d += 2
    raise RuntimeError(f"There are no primes with bitlen at most {bitlen}!")


if __name__ == "__main__":
    import sys

    candidate = find_candidate(int(sys.argv[1]))
    n = int(candidate)
    s = candidate.even_exp
    d = candidate.odd_part
    bits = len(f"{n:b}")
    print(f"(2^{s} * {d} + 1) => {n} ({bits} bits) is prime")
