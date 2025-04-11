#include <stdint.h>
#include <stdio.h>

int main() {
  uint16_t aabb = 0xAABB;
  switch (*(uint8_t *)&aabb) {
  case 0xAA:
    puts("big\n");
    return 0;
  case 0xBB:
    puts("little\n");
    return 0;
  default:
    return -1;
  }
}
