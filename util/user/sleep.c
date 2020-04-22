#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char **argv)
{
  uint ticksCount;

  if (argc < 2) {
    fprintf(2, "usage: sleep numbers_of_ticks\n");
    exit();
  }

  ticksCount = (uint)atoi(argv[1]);
  sleep(ticksCount);

  exit();
}
