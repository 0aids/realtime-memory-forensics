from .rmf_core_py import *
from os import cpu_count
from typing import get_type_hints
from collections.abc import Iterable
from operator import itemgetter
from itertools import repeat
from multiprocessing.pool import ThreadPool as TPool

# Write some generic code here replacing
# templated code such as Analyzer::execute.


class Const:
    def __init__(self, value):
        self.value = value


class Iter:
    def __init__(self, value):
        if not isinstance(value, Iterable):
            raise TypeError("Iter concept should be iterable")
        self.value = value


class Analyzer:
    def __init__(self, numThreads: int = int(cpu_count() / 2)):
        self.numThreads = numThreads

    def execute(self, function, *kargs):
        currLen: int = 0
        listOfIterables = []
        print(f"{function=}, {kargs=}")
        # Just for error checking
        for arg in kargs:
            if isinstance(arg, Const):
                listOfIterables.append(repeat(arg.value))
            elif isinstance(arg, Iter):
                listOfIterables.append(arg.value)
            else:
                raise TypeError(
                    f"Arguments are not explicitly states as const or iterable"
                )
        print(f"Num Arguments passed: {len(listOfIterables)}")
        with TPool(processes=self.numThreads) as pool:
            result = pool.starmap(function, zip(*listOfIterables))
            return result
        return []

