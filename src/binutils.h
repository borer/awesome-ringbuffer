#ifndef BINUTILS_H
#define BINUTILS_H

#define ALIGNMENT (2 * sizeof(int32_t))

#define IS_POWER_OF_TWO(value) ((value) > 0 && (((value) & (~(value) + 1)) == (value)))

#define ALIGN(value, alignment) (((value) + ((alignment) - 1)) & ~((alignment) - 1))

#endif // BINUTILS_H
