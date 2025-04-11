from .. import HeaderBuilder
from .euclid import inverse
from .find_gen import get_mod_generator
from .find_prime import find_candidate


def log2_floor(x: int):
    return len(f"{x:b}") - 1


def log2_ceil(x: int):
    is_pow2 = not x & (x - 1)
    return len(f"{x:b}") - is_pow2


class WordSplit:

    def __init__(self, x: int, *, word_bits=64):
        def gen():
            nonlocal x
            mask = (1 << word_bits) - 1
            while x:
                yield x & mask
                x >>= word_bits

        self.__words = list(gen())
        self.__word_bits = word_bits

    def words(self, bitlen: int):
        padding = max(1, bitlen // self.__word_bits) - len(self.__words)
        assert padding >= 0
        return padding * [0] + list(reversed(self.__words))

    def hex(self, bitlen: int):
        nhex = self.__word_bits // 4
        return list(f"{word:0{nhex}x}" for word in self.words(bitlen))


class HeaderBase(HeaderBuilder):

    DEFAULT_DIGIT_BIT: int

    def build(self):
        self.get_digits()
        self.find_prime()
        self.find_inverse_prime()
        self.find_root_of_unity()
        self.find_half()
        self.find_radix()

    def display_hex(self, x: int, *, bitlen: int | None = None):
        return "0x{}".format("_".join(WordSplit(x).hex(bitlen or self.digit_bit)))

    def get_digits(self):
        def uint_macro(bitlen: int, wordlen=64):
            def gen(width: int, wordlen: int):
                i = 0
                shift = 0
                while width > 0:
                    yield (f"x{i}", shift)
                    i += 1
                    shift += wordlen
                    width -= wordlen

            xs = list(reversed(list(gen(bitlen, wordlen))))
            self.writelines(
                "#define UINT{bitlen}({xargs}) {defn}".format(
                    bitlen=bitlen,
                    xargs=", ".join(x for x, _ in xs),
                    defn=" | ".join(
                        f"((unsigned _BitInt({bitlen}))({x}) << {shift})"
                        for x, shift in xs
                    ),
                )
            )

        self.digit_bit = self.macro("DIGIT_BIT", int, self.DEFAULT_DIGIT_BIT)
        self.dbdgt_bit = self.macro("DBDGT_BIT", int, self.digit_bit * 2, override=True)
        self.qudgt_bit = self.macro("QUDGT_BIT", int, self.digit_bit * 4, override=True)
        self.info(f"Digit width: {self.digit_bit}")

        uint_macro(self.digit_bit)
        uint_macro(self.dbdgt_bit)
        uint_macro(self.qudgt_bit)

        self.writelines(
            "",
            "typedef unsigned _BitInt(DIGIT_BIT) digit_t;",
            "typedef unsigned _BitInt(DBDGT_BIT) dbdgt_t;",
            "typedef unsigned _BitInt(QUDGT_BIT) qudgt_t;",
        )

    def def_int(self, x: int, bitlen: int):
        return "UINT{bitlen}({xs})".format(
            bitlen=bitlen, xs=", ".join(f"0x{h}" for h in WordSplit(x).hex(bitlen))
        )

    def find_prime(self):
        self.prime = find_candidate(self.digit_bit)
        self.prime_int = int(self.prime)
        self.info(f"Using prime: 2^{self.prime.even_exp} * {self.prime.odd_part} + 1")
        self.info(f"Prime hex: {self.display_hex(self.prime_int)}")

        self.writelines(
            "",
            f"static digit_t const prime = {self.def_int(self.prime_int, self.digit_bit)};",
        )

    def find_root_of_unity(self):
        gen = get_mod_generator(self.prime_int)
        self.info(f"Using unit generator: {self.display_hex(gen)}")

        self.omega = pow(gen, self.prime.odd_part, self.prime_int)
        self.info(
            f"Primitive 2^{self.prime.even_exp}th root of unity: {self.display_hex(self.omega)}"
        )

        roots_of_unity = list(self.gen_dyadic_powers(self.omega))
        assert len(roots_of_unity) == self.prime.even_exp + 1

        self.writelines(
            "",
            "// `root_of_unity[k]` is a primitive 2^k-th root of unity",
            "static digit_t const root_of_unity[] = {",
            *(
                f"  {self.def_int(root, self.digit_bit)},"
                for root in reversed(roots_of_unity)
            ),
            "};",
        )

        self.ominv = pow(self.omega, (1 << self.prime.even_exp) - 1, self.prime_int)
        self.info(
            f"Conjugate 2^{self.prime.even_exp}th root of unity: {self.display_hex(self.ominv)}"
        )

        conjs_of_unity = list(self.gen_dyadic_powers(self.ominv))
        assert len(conjs_of_unity) == self.prime.even_exp + 1

        self.writelines(
            "",
            "// `conj_of_unity[k]` is the inverse of `root_of_unity[k]`",
            "static digit_t const conj_of_unity[] = {",
            *(
                f"  {self.def_int(conj, self.digit_bit)},"
                for conj in reversed(conjs_of_unity)
            ),
            "};",
        )

    def find_half(self):
        self.half = inverse(2, self.prime_int)
        self.info(f"2^-1: {self.display_hex(self.half)}")

        self.writelines(
            "",
            "// `power_of_half[k]` is 2^(-k)",
            "static digit_t const power_of_half[] = {",
            *(
                f"  {self.def_int(half, self.digit_bit)},"
                for _, half in zip(
                    range(self.prime.even_exp + 1), self.gen_powers(self.half)
                )
            ),
            "};",
        )

    def find_inverse_prime(self):
        log = log2_ceil(self.prime_int)
        self.prime_shift = self.dbdgt_bit + log
        num = 1 << self.prime_shift
        self.prime_mu = (num + self.prime_int - 1) // self.prime_int
        while self.prime_mu & 1 == 0:
            self.prime_mu >>= 1
            self.prime_shift -= 1

        mu_is_dbdgt = len(f"{self.prime_mu:b}") <= self.dbdgt_bit
        if mu_is_dbdgt:
            quot = ("  dbdgt_t quot = ((qudgt_t)x * prime_mu) >> prime_shift;",)
            self.info(
                "Division by prime converted to:",
                f"1. Multiplication by {self.display_hex(self.prime_mu, bitlen=self.dbdgt_bit)}",
                f"2. Right shift by {self.prime_shift}",
            )
        else:
            self.prime_mu &= (1 << self.dbdgt_bit) - 1
            quot = (
                "  dbdgt_t prod = ((qudgt_t)x * prime_mu) >> DBDGT_BIT;",
                "  dbdgt_t carry = ((dbdgt_t)__builtin_add_overflow(prod, x, &prod)) << (DBDGT_BIT - (prime_shift - DBDGT_BIT));",
                "  dbdgt_t quot = (prod >> (prime_shift - DBDGT_BIT)) | carry;",
            )
            self.info(
                "Division by prime converted to:",
                f"1. Multiplication by {self.display_hex(self.prime_mu, bitlen=self.dbdgt_bit)}",
                f"2. Right shift by {self.dbdgt_bit}",
                "3. Add original value",
                f"4. Right shift by {self.prime_shift - self.dbdgt_bit}",
            )

        self.writelines(
            "",
            "// Reciprocal of prime approximation",
            f"static dbdgt_t const prime_mu = {self.def_int(self.prime_mu, self.dbdgt_bit)};",
            f"static unsigned const prime_shift = {self.prime_shift};",
            "",
            "static digit_t",
            "mod_prime(dbdgt_t x)",
            "{",
            *quot,
            "  return x - prime * quot;",
            "}",
            "",
            "#define PAD(x) ((dbdgt_t)(x))",
            "#define PAD_OP(x, op ,y) (PAD(x) op PAD(y))",
            "#define MOD_OP(x, op, y) mod_prime(PAD_OP(x, op, y))",
            "#define MOD_MUL(x, y) MOD_OP(x, *, y)",
            "#define MOD_ADD(x, y) MOD_OP(x, +, y)",
            "#define MOD_NEG(x) MOD_OP(prime - 1, *, x)",
            "#define MOD_SUB(x, y) MOD_OP(x, +, MOD_NEG(y))",
        )

    def find_radix(self):
        improper_digit_max_bitlen = len(f"{self.prime_int:b}") - 1
        max_radix_bit = (improper_digit_max_bitlen - 4) // 2
        self.radix_bit = 1 << log2_floor(max_radix_bit)
        expansion = self.digit_bit // self.radix_bit
        self.expansion_exp = log2_ceil(expansion)

        self.writelines("")
        self.macro("RADIX_BIT", int, self.radix_bit, override=True)
        self.writelines(
            "typedef unsigned _BitInt(RADIX_BIT) radix_t;",
            "",
            "// sizeof(digit_t) == sizeof(radix_t) << expansion_exp",
            f"static unsigned const expansion_exp = {self.expansion_exp};",
        )

        self.info(f"Radix width: {self.radix_bit}")

        soft_max_fib_size = improper_digit_max_bitlen - 3 - 2 * self.radix_bit
        soft_max_fib_index = 4 * ((1 << soft_max_fib_size) * self.radix_bit + 1) // 3
        warning = f"No promises for Fibonacci numbers with index above {soft_max_fib_index:_d}"
        self.warn(warning)
        self.writelines("", f"// {warning}")

    def gen_powers(self, elt: int):
        x = 1
        while True:
            yield x
            x = (x * elt) % self.prime_int

    def gen_dyadic_powers(self, elt: int):
        x = elt
        while True:
            yield x
            if x == 1:
                return
            x = (x * x) % self.prime_int
