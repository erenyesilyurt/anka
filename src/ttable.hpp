#pragma once
#include "core.hpp"
#include "engine_settings.hpp"
#include "move.hpp"
#include "gamestate.hpp"
#include "movegen.hpp"
#include <string.h>

namespace anka {

	enum class NodeType { PV, UPPERBOUND, LOWERBOUND };

	// the result of a successful transposition table probe
	struct TTResult {
		Move move;
		int depth;
		i16 score;
		NodeType type;
	};

	// Lockless shared transposition table
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
			m_table_hits = 0;
		}

		inline bool Init(size_t hash_size)
		{
			if (hash_size < 1)
				return false;
			
			auto table_size_old = m_table_size;
			auto num_buckets_old = m_num_buckets;

			m_num_buckets = (hash_size * MB ) / sizeof(TTBucket);
			m_table_size = m_num_buckets * sizeof(TTBucket);


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

		inline bool Get(const u64 pos_key, TTResult &result)
		{
			ANKA_ASSERT(pos_key != C64(0));

			size_t bucket_index = pos_key % m_num_buckets;
			
			auto bucket = m_table[bucket_index];

			for (int i = 0; i < 4; i++) {
				if ((bucket.records[i].key ^ bucket.records[i].data) == pos_key) {
					result.move = (bucket.records[i].data & 0xffffffff);
					result.score = (bucket.records[i].data >> 32) & 0xffff;
					result.depth = (bucket.records[i].data >> 48) & 0xff;
					result.type = static_cast<NodeType>((bucket.records[i].data >> 56) & 0x3);

					ANKA_ASSERT(result.score <= EngineSettings::MAX_SCORE && result.score >= EngineSettings::MIN_SCORE);
					ANKA_ASSERT(result.move != 0);
					ANKA_ASSERT(result.depth <= EngineSettings::MAX_DEPTH && result.depth > 0);

					return true;
				}
			}

			return false;
		}

		inline void Put(u64 pos_key, NodeType type, int depth, Move best_move, i16 score)
		{
			ANKA_ASSERT(score <= EngineSettings::MAX_SCORE && score >= EngineSettings::MIN_SCORE);
			ANKA_ASSERT(pos_key != C64(0));
			ANKA_ASSERT(best_move != 0);
			ANKA_ASSERT(depth <= EngineSettings::MAX_DEPTH && depth >= 1);

			size_t bucket_index = pos_key % m_num_buckets;
			u64 score_bits = score & 0xffff;
			u64 depth_bits = depth & 0xff;
			u64 node_bits = (int)type & 0x3;
			u64 age_bits = m_current_age & 0x1f;

			u64 data = best_move;
			data |= (score_bits << 32);
			data |= (depth_bits << 48);
			data |= (node_bits << 56);
			data |= (age_bits << 58);
			u64 key = pos_key ^ data;

			int chosen_record_index = 0;
			int min_depth = EngineSettings::MAX_DEPTH;
			for (int i = 0; i < 4; i++) {
				if (age_bits != ((m_table[bucket_index].records[i].data >> 58) & 0x1f)) {
					chosen_record_index = i;
					break;
				}
				int record_depth = (m_table[bucket_index].records[i].data >> 48) & 0xff;
				if (record_depth < min_depth) {
					min_depth = record_depth;
					chosen_record_index = i;
				}
			}

			m_table[bucket_index].records[chosen_record_index] = { key, data };
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

				if (Get(pos_key, node) && node.type == NodeType::PV) {
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
			m_current_age++;
		}

	// TODO: 4 cells per bucket
	private:
		/**
		 * TTRecord Encoding
		 * key:
		 *	positionkey ^ data
		 * data:
		 *	[0:31]  : best move // 32 bits
		 *  [32:47] : score // 16 bits
		 *  [48:55] : depth // 8 bits
		 *  [56:57] : node type // 2 bits
		 *  [58:62] : age // 5 bits
		*/

		// Each record is 16 bytes
		struct TTRecord {
			u64 key; // key is position key ^ data
			u64 data;
		};

		// Each bucket is 64 bytes
		struct TTBucket {
			TTRecord records[4];
		};

		static constexpr size_t MB = 1000000;
		size_t m_num_buckets; // number of TTBuckets
		size_t m_table_size; // total table size in bytes
		u64 m_table_hits;
		TTBucket* m_table;
		unsigned int m_current_age;
	};

	extern TranspositionTable trans_table;

}