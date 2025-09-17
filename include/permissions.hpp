#pragma once
#include <string>

typedef unsigned char PermissionsMask;

enum RegionPermissions {
  READ = 1 << 0,
  WRITE = 1 << 1,
  EXECUTE = 1 << 2,
  PRIVATE = 1 << 3,
};

#define mapLocationFromPID(PID) "/proc/" + std::to_string(PID) + "/maps"
#define memLocationFromPID(PID) "/proc/" + std::to_string(PID) + "/mem"

#define isReadable(permissions) ((permissions & READ) != 0)
#define isWritable(permissions) ((permissions & WRITE) != 0)

static std::string ungeneratePermissionsMask(PermissionsMask perms) {
#define R 0
#define W 1
#define X 2
#define P 3
  std::string mask;
  mask.resize(5);
  if ((perms & READ) != 0) {
    mask[R] = 'r';
  }
  if ((perms & WRITE) != 0) {
    mask[W] = 'w';
  }
  if ((perms & EXECUTE) != 0) {
    mask[X] = 'x';
  }
  if ((perms & PRIVATE) != 0) {
    mask[P] = 'p';
  }
#undef R
#undef W
#undef X
#undef P

  return mask;
}

static PermissionsMask generatePermissionsMask(char perms[4]) {
#define R 0
#define W 1
#define X 2
#define P 3
  PermissionsMask mask = 0;
  if (perms[R] == 'r') {
    mask |= READ;
  }
  if (perms[W] == 'w') {
    mask |= WRITE;
  }
  if (perms[X] == 'x') {
    mask |= EXECUTE;
  }
  if (perms[P] == 'p') {
    mask |= PRIVATE;
  }
#undef R
#undef W
#undef X
#undef P

  return mask;
}
