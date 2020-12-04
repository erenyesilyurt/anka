#include <stdio.h>
#include <string.h>
#include "bitboard.hpp"
#include "attacks.hpp"
#include "rng.hpp"
using namespace anka;


u64 FindMagic(Square square, bool is_rook, bool eval_for_size = true, unsigned int max_iterations = 100)
{
  RNG rng(420);
  u64 best_magic = C64(0);
  int best_score = -1;

  static u64 buckets[4096] {};
  static u64 occupations[4096] {};
  static u64 piece_attacks[4096] {};

  auto mask = is_rook ? attacks::magics::rook_magics[square].mask
                      : attacks::magics::bishop_magics[square].mask;
  int shift = is_rook ? (64 - 12) : (64 - 9);

  // generate all possible relevant occupations and corresponding attacks
  int num_mask_bits = bitboard::PopCount(mask);
  int num_occupations = (1 << num_mask_bits);
  for (Bitboard selection = 0; selection < num_occupations; selection++) {
    occupations[selection] = bitboard::BitSubset(mask, selection);
    piece_attacks[selection] = is_rook ? attacks::RookAttacksSlow(square, occupations[selection])
                                       : attacks::BishopAttacksSlow(square, occupations[selection]);
  }


  for (int i = 0; i < max_iterations; i++) {
    u64 magic = rng.rand64_sparse();
    if (bitboard::PopCount((magic * mask) & C64(0xFF00000000000000)) < 6)
      continue;
    memset(buckets, 0, 4096 * sizeof(u64));
    int max_index = -1;
    bool success = true;
    for (int j = 0; j < num_occupations; j++) {
      auto index = static_cast<int>((occupations[j] * magic) >> shift);
      if (index > max_index)
        max_index = index;

      if (buckets[index] == 0) {
        buckets[index] = piece_attacks[j];
      }
      else if (buckets[index] != piece_attacks[j]) {
        success = false;
        break; // index collision
      }
    }

    if (success) {
      int score = -1;
      if (eval_for_size) {
        score = 4095 - max_index;
      }
      else { // eval for max gap
        int j = 0;
        for (j = 0; j < num_occupations; j++) {
          if (buckets[j] != 0)
            break;
        }
        int previous = j;
        for (j = j + 1; j < num_occupations; j++) {
          if (buckets[j] == 0)
            continue;
          int gap = j - previous;
          previous = j;
          if (gap > score)
            score = gap;
        }

      }

      if (score > best_score) {
        best_score = score;
        best_magic = magic;
      }
    }
  }
 std::cout << best_magic << ' ' << best_score << '\n';
  return best_magic;
}

int main()
{
  
  for (int sq = 0; sq < 64; sq ++) {
  u64 magic = FindMagic(sq, true, false, 10000000);
  
  }

  std::cout << std::endl;
  return 0;
}