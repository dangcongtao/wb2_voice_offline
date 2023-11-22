#ifndef IRSEND_H_
#define IRSEND_H_



typedef enum {
  GE6711AR2853M = 1,  // (1) LG 28-bit Protocol (default)
  AKB75215403,        // (2) LG2 28-bit Protocol
  AKB74955603,        // (3) LG2 28-bit Protocol variant
  AKB73757604,        // (4) LG2 Variant of AKB74955603
  LG6711A20083V,      // (5) Same as GE6711AR2853M, but only SwingV toggle.
} lg_ac_remote_model_t;

#endif