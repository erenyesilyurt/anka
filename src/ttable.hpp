#pragma once
#include "core.hpp"
#include "engine_settings.hpp"
#include "move.hpp"
#include "gamestate.hpp"
#include "movegen.hpp"
#include <string.h>

namespace anka {

	enum class NodeType { EXACT, UPPERBOUND, LOWERBOUND };

	// the result of a successful transposition table probe
	struct TTResult {
		Move move;
		int depth;
		i16 value;
		NodeType type;
	};

	class TranspositionTable {
	public:
		TranspositionTable() : m_table {nullptr}, m_num_buckets(0), m_table_size(0), m_table_hits(0), m_current_age(0) {}
		~TranspositionTable()
		{
			free(m_table);
		}

		inline void Clear()
		{
			memset(m_table, 0, m_table_size);
			m_current_age = 0;
			m_table_hits = 0;
		}

		inline bool Init(size_t hash_size)
		{
			if (hash_size < EngineSettings::MIN_HASH_SIZE || hash_size > EngineSettings::MAX_HASH_SIZE)
				return false;

			auto table_size_old = m_table_size;
			auto num_buckets_old = m_num_buckets;

			m_table_size = 1 * MiB;
			// round down hash size to nearest power of two
			while (hash_size >>= 1) {
				m_table_size <<= 1;
			}
			m_num_buckets = m_table_size / sizeof(TTBucket);

			TTBucket* table = m_table;
			// allocate
			table = (TTBucket*)realloc(m_table, m_table_size);

			if (table == nullptr) {
				m_table_size = table_size_old;
				m_num_buckets = num_buckets_old;
				return false;
			}
			else {
				m_table = table;
				Clear();
				return true;
			}
		}

		inline bool Get(u64 pos_hash, TTResult &result)
		{
			ANKA_ASSERT(pos_hash != C64(0));
			size_t bucket_index = pos_hash & (m_num_buckets-1);
			u32 pos_key = static_cast<u32>(pos_hash >> 32);

			auto bucket = m_table[bucket_index];
			for (int i = 0; i < num_cells; i++) {
				if (bucket.records[i].key == pos_key) {
					result.move = bucket.records[i].move;
					result.value = bucket.records[i].value;
					result.depth = bucket.records[i].depth;
					result.type = static_cast<NodeType>((bucket.records[i].node_type_and_age >> 6));

					ANKA_ASSERT(result.value <= EngineSettings::MAX_SCORE && result.value >= EngineSettings::MIN_SCORE);
					ANKA_ASSERT(result.move != 0);
					ANKA_ASSERT(result.depth <= EngineSettings::MAX_DEPTH && result.depth > 0);

					return true;
				}
			}

			return false;
		}

		inline void Put(u64 pos_hash, NodeType type, int depth, Move best_move, i16 value)
		{
			ANKA_ASSERT(value <= EngineSettings::MAX_SCORE && value >= EngineSettings::MIN_SCORE);
			ANKA_ASSERT(pos_hash != C64(0));
			ANKA_ASSERT(best_move != 0);
			ANKA_ASSERT(depth <= EngineSettings::MAX_DEPTH && depth >= 1);

			size_t bucket_index = pos_hash & (m_num_buckets - 1);
			u32 pos_key = static_cast<u32>(pos_hash >> 32);

			int chosen_index = 0;
			int min_depth = EngineSettings::MAX_DEPTH;
			for (int i = 0; i < num_cells; i++) {
				int record_age = m_table[bucket_index].records[i].node_type_and_age & 0x3f;
				if (record_age != m_current_age) {
					chosen_index = i;
					break;
				}

				if (m_table[bucket_index].records[i].depth < min_depth) {
					min_depth = m_table[bucket_index].records[i].depth;
					chosen_index = i;
				}
			}

			int type_and_age = static_cast<int>(type);
			type_and_age = (type_and_age << 6) | m_current_age;
			m_table[bucket_index].records[chosen_index].key = pos_key;
			m_table[bucket_index].records[chosen_index].value = value;
			m_table[bucket_index].records[chosen_index].depth = depth;
			m_table[bucket_index].records[chosen_index].move = best_move;
			m_table[bucket_index].records[chosen_index].node_type_and_age = type_and_age;
		}

		// extracts the principal variation moves into pv. returns the number of extracted moves.
		inline int GetPrincipalVariation(GameState &pos, Move *pv, int max_pv_length)
		{
			MoveList<256> list;
			TTResult node;
			int moves_made = 0;

			while (true) {
				u64 pos_key = pos.PositionKey();
				list.GenerateLegalMoves(pos);

				if (Get(pos_key, node) && node.type == NodeType::EXACT) {
					// check move for legality
					if (node.move == 0 || moves_made == max_pv_length)
						break;
					if (list.Find(node.move)) {
						pv[moves_made] = node.move;
						pos.MakeMove(node.move);
						moves_made++;
					}
					else {
						break;
					}
				}
				else {
					break;
				}
			}

			for (int i = 0; i < moves_made; i++) {
				pos.UndoMove();
			}
			
			return moves_made;
		}

		inline void IncrementAge()
		{
			m_current_age = ++m_current_age & 0x3f;
		}

		inline int GetAge() const {
			return m_current_age;
		}

	private:
		static constexpr int num_cells = 5;
		static constexpr size_t MiB = 1'048'576;

		/**
		 * TTRecord Encoding
		 * key:
		 *	LSB32 of positionkey // 32 bits
		 * data:
		 *	[0:31]  : best move // 32 bits
		 *  [32:47] : value // 16 bits
		 *  [48:55] : depth // 8 bits
		 *  [56:63] : node type and age // 2 bits and 6 bits
		*/

		// Each record is 12 bytes
		struct TTRecord {
			u32 key;
			u32 move;
			i16 value;
			byte depth;
			byte node_type_and_age;
		};
		static_assert(sizeof(TTRecord) == 12, "TTRecord: unexpected struct alignment");

		// Each bucket is 64 bytes
		struct TTBucket {
			TTRecord records[num_cells];
			u32 padding;
		};
		static_assert(sizeof(TTBucket) == 64, "TTBucket: unexpected size");


		size_t m_num_buckets; // number of TTBuckets
		size_t m_table_size; // total table size in bytes
		u64 m_table_hits;
		TTBucket* m_table;
		unsigned int m_current_age;
		
	};

	extern TranspositionTable trans_table;

}