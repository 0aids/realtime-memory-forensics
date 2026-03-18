from .rmf_core_py import *
from os import cpu_count
from collections.abc import Iterable
from itertools import repeat
from multiprocessing.pool import ThreadPool as TPool
from collections import UserList
from itertools import chain

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


class AnalyzerResult(UserList):
    def flatten(self):
        if len(self) == 0:
            raise IndexError("The AnalyzerResult is empty! Cannot be flattened")
        if isinstance(self[0], MemoryRegionPropertiesVec):
            # specific case in which we need to call bindings
            # bc opaque type.
            return ConsolidateMrpVec(self)
        try:
            print(type(self[0][0]))
        except IndexError:
            raise IndexError("The AnalyzerResult value is not 2+ degrees nested")

        return AnalyzerResult(chain.from_iterable(self))


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
                    "Arguments are not explicitly stated as const or iterable"
                )
        if len(kargs) == 1 and isinstance(kargs[0], Const):
            listOfIterables = [[kargs[0].value]]
        print(f"Num Arguments passed: {len(listOfIterables)}")
        result = AnalyzerResult()
        with TPool(processes=self.numThreads) as pool:
            result.extend(pool.starmap(function, zip(*listOfIterables)))
            return result
        return result
