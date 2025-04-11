import os

from .num_theory.header import HeaderBase


class Header(HeaderBase):
    DEFAULT_DIGIT_BIT = 64

    def build(self):
        self.nproc()
        super().build()

    def nproc(self):
        cpu_count = getattr(os, "process_cpu_count", os.cpu_count)()
        self.cpu_count = self.macro("NPROC", int, cpu_count)
        self.nproc_log = self.macro(
            "NPROC_LOG", int, len(f"{self.cpu_count:b}") - 1, override=True
        )
        self.info(
            f"Configuring for {1<<self.nproc_log} thread(s) [{cpu_count} available]."
        )
