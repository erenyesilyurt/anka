#pragma once
#include "core.hpp"
#include "engine_settings.hpp"
#include "move.hpp"
#include "gamestate.hpp"
#include "movegen.hpp"
#include <string.h>
#include <inttypes.h>
namespace anka {

	enum class NodeType { EXACT, UPPERBOUND, LOWERBOUND, NONE };

	// Each record is 12 bytes
	struct TTRecord {
		u32 key;
		u32 move;
		i16 value;
		byte depth;
		byte node_type_and_age;

		force_inline NodeType GetNodeType() const
		{
			return static_cast<NodeType>((node_type_and_age >> 6));
		}

		force_inline int GetAge() const
		{
			return node_type_and_age & 0x3f;
		}
	};
	static_assert(sizeof(TTRecord) == 12, "TTRecord: unexpected struct alignment");

	class TranspositionTable {
	public:
		TranspositionTable() : m_table {nullptr}, m_num_buckets(0), m_table_size(0), m_table_hits(0), m_num_queries(0), m_current_age(0) {}
		~TranspositionTable()
		{
			free(m_table);
		}

		void Clear()
		{
			memset(m_table, 0, m_table_size);
			m_current_age = 0;
			m_table_hits = 0;
			m_num_queries = 0;
		}

		bool Init(size_t hash_size)
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

		bool Get(u64 pos_hash, TTRecord &result, int ply)
		{
			ANKA_ASSERT(pos_hash != C64(0));
			STATS(m_num_queries++);
			size_t bucket_index = pos_hash & (m_num_buckets-1);
			u32 pos_key = static_cast<u32>(pos_hash >> 32);

			for (int i = 0; i < num_cells; i++) {
				if (m_table[bucket_index].records[i].key == pos_key) {
					result = m_table[bucket_index].records[i];
					if (result.value > UPPER_MATE_THRESHOLD) {
						result.value -= ply;
					}
					else if (result.value < LOWER_MATE_THRESHOLD) {
						result.value += ply;
					}
					STATS(m_table_hits++);
					return true;
				}
			}

			return false;
		}

		void Put(u64 pos_hash, NodeType type, int depth, Move best_move, i16 value, int ply, bool timeup)
		{
			if (timeup)
				return;
			ANKA_ASSERT(pos_hash != C64(0));
			//ANKA_ASSERT(best_move != 0);
			ANKA_ASSERT(depth <= MAX_DEPTH && depth >= 1);

			size_t bucket_index = pos_hash & (m_num_buckets - 1);
			u32 pos_key = static_cast<u32>(pos_hash >> 32);

			int chosen_index = -1;
			int min_depth = MAX_DEPTH + 1;
			// replacement strategy: same entry > different age > lower depth
			for (int i = 0; i < num_cells; i++) {
				if (pos_key == m_table[bucket_index].records[i].key) {
					chosen_index = i;
					break;
				}
				int record_age = m_table[bucket_index].records[i].GetAge();
				if (record_age != m_current_age) {
					chosen_index = i;
				}

				if (chosen_index == - 1 && m_table[bucket_index].records[i].depth < min_depth) {
					min_depth = m_table[bucket_index].records[i].depth;
					chosen_index = i;
				}
			}

			int type_and_age = static_cast<int>(type);
			type_and_age = (type_and_age << 6) | m_current_age;
			if (value > UPPER_MATE_THRESHOLD) {
				value += ply;
			}
			else if (value < LOWER_MATE_THRESHOLD) {
				value -= ply;
			}
			m_table[bucket_index].records[chosen_index].key = pos_key;
			m_table[bucket_index].records[chosen_index].value = value;
			m_table[bucket_index].records[chosen_index].depth = depth;
			m_table[bucket_index].records[chosen_index].move = best_move;
			m_table[bucket_index].records[chosen_index].node_type_and_age = type_and_age;
		}

		// extracts the principal variation moves into pv. returns the number of extracted moves.
		int ExtractPV(GameState &pos, Move root_best_move, Move *pv, int max_pv_length)
		{
			MoveList<256> list;
			TTRecord node;

			pv[0] = root_best_move;
			pos.MakeMove(root_best_move);
			int moves_made = 1;

			while (true) {
				u64 pos_key = pos.PositionKey();
				list.GenerateLegalMoves(pos);

				if (Get(pos_key, node, 0) && node.GetNodeType() == NodeType::EXACT) {
					// check move for legality
					if (node.move == 0 || moves_made == max_pv_length)
						break;
					if (list.Find(node.move)) {
						pv[moves_made] = node.move;
						pos.MakeMove(node.move);
						moves_made++;
						if (pos.IsDrawn()) // detect repetitions
							break;
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

		void IncrementAge()
		{
			m_current_age = ++m_current_age & 0x3f;
		}

		int GetAge() const 
		{
			return m_current_age;
		}

		#ifdef STATS_ENABLED
		void PrintStatistics() const
		{
			u64 n_used_buckets = 0;
			u64 n_total_used_cells = 0;
			u64 n_exact = 0;
			u64 n_lowerbound = 0;
			u64 n_upperbound = 0;

			for (size_t b = 0; b < m_num_buckets; b++) {
				auto& bucket = m_table[b];

				int n_used_cells = 0;
				for (int r = 0; r < num_cells; r++) {
					if (bucket.records[r].key > 0) {
						n_used_cells++;
						n_total_used_cells++;

						switch (bucket.records[r].GetNodeType())
						{
						case NodeType::EXACT:
							n_exact++;
							break;
						case NodeType::LOWERBOUND:
							n_lowerbound++;
							break;
						case NodeType::UPPERBOUND:
							n_upperbound++;
							break;
						default:
							break;
						}
					}
				}

				if (n_used_cells > 0)
					n_used_buckets++;
			}

			printf("\nHASH TABLE STATISTICS\n");
			printf("Hit rate: %.2f\n", m_table_hits / static_cast<double>(m_num_queries));
			printf("Load factor (n_used_buckets / total_buckets): %.4f\n", n_used_buckets / static_cast<double>(m_num_buckets));
			printf("Load factor (n_used_cells / total_buckets): %.4f\n", n_total_used_cells / static_cast<double>(m_num_buckets));
			printf("Exact: %" PRIu64 "\n", n_exact);
			printf("Lowerbound: %" PRIu64 "\n", n_lowerbound);
			printf("Upperbound: %" PRIu64 "\n", n_upperbound);
			double exact_perc = (n_exact / static_cast<double>(n_total_used_cells)) * 100.0;
			double lower_perc = (n_lowerbound / static_cast<double>(n_total_used_cells)) * 100.0;
			double upper_perc = (n_upperbound / static_cast<double>(n_total_used_cells)) * 100.0;
			printf("EXACT: %.2f\%%, LB: %.2f\%%, UB: %.2f\%%\n", exact_perc, lower_perc, upper_perc);
		}
		#endif

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

		// Each bucket is 64 bytes
		struct TTBucket {
			TTRecord records[num_cells];
			u32 padding;
		};
		static_assert(sizeof(TTBucket) == 64, "TTBucket: unexpected size");


		size_t m_num_buckets; // number of TTBuckets
		size_t m_table_size; // total table size in bytes
		u64 m_table_hits;
		u64 m_num_queries;
		TTBucket* m_table;
		unsigned int m_current_age;
		
	};

	extern TranspositionTable g_trans_table;

}