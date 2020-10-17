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
	class TransposTable {
	public:
		TransposTable() : m_table {nullptr}, m_table_length(0), m_table_size(0), m_table_hits(0) {}
		~TransposTable()
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
			auto table_length_old = m_table_length;

			m_table_length = (hash_size * MB ) / sizeof(TTRecord);
			m_table_size = m_table_length * sizeof(TTRecord);


			TTRecord* table = m_table;
			// allocate / reallocate
			if (table == nullptr)
				table = (TTRecord*)realloc(NULL, m_table_size);
			else {		
				table = (TTRecord*)realloc(m_table, m_table_size);
			}

			if (table == nullptr) {
				m_table_size = table_size_old;
				m_table_length = table_length_old;
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

			size_t index = pos_key % m_table_length;
			u64 data = m_table[index].data;

			if ((m_table[index].key ^ data) != pos_key)
				return false;
	
			// extract node info from data
			result.move = data & 0xffffffff;
			result.score = (data & 0xffff00000000) >> 32;
			result.depth = (data & 0xff000000000000) >> 48;
			result.type = static_cast<NodeType>((data & 0x300000000000000) >> 56);

			ANKA_ASSERT(result.score <= EngineSettings::MAX_SCORE && result.score >= EngineSettings::MIN_SCORE);
			ANKA_ASSERT(result.move != 0);
			ANKA_ASSERT(result.depth <= EngineSettings::MAX_DEPTH && result.depth > 0);


			return true;
		}

		inline void Set(u64 pos_key, NodeType type, int depth, Move best_move, i16 score)
		{
			ANKA_ASSERT(score <= EngineSettings::MAX_SCORE && score >= EngineSettings::MIN_SCORE);
			ANKA_ASSERT(pos_key != C64(0));
			ANKA_ASSERT(best_move != 0);
			ANKA_ASSERT(depth <= EngineSettings::MAX_DEPTH && depth >= 1);

			size_t index = pos_key % m_table_length;
			u64 score_bits = score & 0xffff;
			u64 depth_bits = depth & 0xff;
			u64 node_bits = (int)type & 0x3;

			u64 data = best_move;
			data |= (score_bits << 32);
			data |= (depth_bits << 48);
			data |= (node_bits << 56);
			u64 key = pos_key ^ data;

			m_table[index].key = key;
			m_table[index].data = data;
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
		*/
		struct TTRecord {
			u64 key; // key is position key ^ data
			u64 data;
		};

		static constexpr size_t MB = 1000000;
		size_t m_table_length; // number of TTRecords
		size_t m_table_size; // total table size in bytes
		u64 m_table_hits = 0;
		TTRecord* m_table;
	};

	extern TransposTable trans_table;

}