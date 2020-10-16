#pragma once

#include "rng.hpp"

namespace anka {
	namespace zobrist_keys {
		extern u64 piece_keys[2][6][64];
		extern u64 castle_keys[16];
		extern u64 side_key;
		extern u64 ep_keys[65]; // includes NOSQUARE(64)
	} // namespace zobrist_keys

	void InitZobristKeys(RNG& rng);


} // namespace anka