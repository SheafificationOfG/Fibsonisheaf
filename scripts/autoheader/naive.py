from . import HeaderBuilder


class Header(HeaderBuilder):

    def build(self):

        digit_bit = self.macro("DIGIT_BIT", int, 64)

        self.info(f"Digit width: {digit_bit}")

        self.writelines(
            "",
            "typedef unsigned _BitInt(DIGIT_BIT) digit_t;",
        )
