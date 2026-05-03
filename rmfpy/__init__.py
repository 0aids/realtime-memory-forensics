import cppyy as cpp
from pathlib import Path

thisFileDir = Path(__file__).parent.resolve()
projRoot = thisFileDir / ".."
magic_enum_dir = projRoot / "build" / "_deps" / "magic_enum-src" / "include"
rmf_dir = projRoot / "include"
so_file = projRoot / "build" / "liblibrmf_so.so"
all_h = [p for p in rmf_dir.rglob("*.hpp") if p.is_file()]

cpp.add_include_path(magic_enum_dir.as_posix())
cpp.add_include_path(rmf_dir.as_posix())
cpp.load_library(so_file.as_posix())
for h in all_h:
    try:
        print(f"Loading {h}")
        cpp.include(h.as_posix())
    except Exception as e:
        print(f"Failed to load {h} because:\n{e}")
        break

mf = cpp.gbl.RealtimeMemoryForensics
mfl = mf.Logging
mfu = mf.Utils
