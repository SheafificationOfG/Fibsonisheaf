from .. import HeaderBuilder


class HeaderBase(HeaderBuilder):

    DEFAULT_DIGIT_BIT: int

    def build(self):

        digit_bit = self.macro("DIGIT_BIT", int, self.DEFAULT_DIGIT_BIT)
        dbdgt_bit = self.macro("DBDGT_BIT", int, 2 * digit_bit, override=True)

        self.info(f"Digit width: {digit_bit}")

        self.writelines(
            "",
            "typedef unsigned _BitInt(DIGIT_BIT) digit_t;",
            "typedef unsigned _BitInt(DBDGT_BIT) dbdgt_t;",
        )
