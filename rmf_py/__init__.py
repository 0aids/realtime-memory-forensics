from .rmf_core_py import *
from os import cpu_count
from collections.abc import Iterable
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


# Way too fucking slow man.
class Analyzer:
    def __init__(self, numThreads: int = int(cpu_count() / 2)):
        self.numThreads = numThreads

    def __enter__(self):
        return self

    def __exit__(self, *kargs):
        pass

    def execute(self, function, *kargs):
        listOfIterables = []
        print(f"{function=}, {kargs=}")
        for arg in kargs:
            if isinstance(arg, Const):
                listOfIterables.append(repeat(arg.value))
            elif isinstance(arg, Iter):
                listOfIterables.append(arg.value)
            else:
                raise TypeError(
                    "Arguments are not explicitly states as const or iterable"
                )
        if len(kargs) == 1 and isinstance(kargs[0], Const):
            listOfIterables = [[kargs[0].value]]
        print(f"Num Arguments passed: {len(listOfIterables)}")
        with TPool(processes=self.numThreads) as pool:
            result = pool.starmap(function, zip(*listOfIterables))
            return result
        return []
