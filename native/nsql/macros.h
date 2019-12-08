#pragma once

/*
 * Evaluates to the number of elements in a statically-allocated or
 * stack-allocated array.
 */
#define countof(x) (sizeof(x) / sizeof((x)[0]))
