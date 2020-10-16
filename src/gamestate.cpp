#include "gamestate.hpp"
#include <sstream>
#include <inttypes.h>
#include <ctype.h>
#include <iostream>
#include <assert.h>


void anka::GameState::Clear()
{
	for (int i = 0; i < 8; i++) {
		m_piecesBB[i] = C64(0);
	}

	for (int sq = 0; sq < 64; sq++) {
		m_board[sq] = piece_type::NOPIECE;
	}
	m_occupation = C64(0);

	m_ep_target = square::NOSQUARE;
	m_castling_rights = 0;
	m_halfmove_clock = 0;
	m_side = side::LOWERBOUND;
	m_zobrist_key = C64(0);
	m_depth = 0;
	m_total_material = 0;
}

bool anka::GameState::LoadPosition(std::string fen)
{
	if (fen.empty()) {
		std::cerr << "AnkaError (LoadPosition): FEN string is empty.\n";
		return false;
	}

	Clear();

	/*
	0: Piece positions (rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR)
	1: Side to move (w or b)
	2: Castling rights (KQkq, Kq, -, etc.)
	3: En passant target (e3, c6 etc.)
	4: Half-move clock
	5: Full-move clock
	*/
	std::string fen_parts[7];

	std::stringstream ss(fen);
	int num_parts = 0;
	while (std::getline(ss, fen_parts[num_parts], ' ')) {
		++num_parts;
		if (num_parts > 6)
			break;
	}

	// Side to move
	if (fen_parts[1] == "w") {
		m_side = side::WHITE;
	}
	else if (fen_parts[1] == "b") {
		m_side = side::BLACK;
	}
	else {
		std::cerr << "AnkaError (LoadPosition): Failed to parse side to move\n";
		return false;

	}

	// Castling rights
	if (fen_parts[2] != "-") {
		const char* p = fen_parts[2].c_str();
		char c;
		while (c = *p++) {
			switch (c) {
			case 'K':
				m_castling_rights |= castle_wk;
				break;
			case 'Q':
				m_castling_rights |= castle_wq;
				break;
			case 'k':
				m_castling_rights |= castle_bk;
				break;
			case 'q':
				m_castling_rights |= castle_bq;
				break;
			}
		}
	}


	// En passant target
	if (fen_parts[3] != "-") {
		const char *p = fen_parts[3].c_str();
		File f = *p - 'a';
		Rank r = *(p + 1) - '1';
		if (f > file::H|| f < file::A|| r > rank::EIGHT|| r < rank::ONE) {
			std::cerr << "AnkaError (LoadPosition): Error parsing en passant square.\n";
			return false;
		}
		m_ep_target = square::RankFileToSquare(r, f);
	}

	if (num_parts > 4) {
		// Half-move clock
		int half_moves = std::stoi(fen_parts[4]);
		if (half_moves < 0) {
			std::cerr << "AnkaError (LoadPosition): Error parsing half move clock.\n";
			return false;
		}
		m_halfmove_clock = half_moves;
	}
	// Piece positions
	// TODO: do more error checking here, otherwise a malformed FEN string will crash the program
	const char* p = fen_parts[0].c_str();
	char c;
	int r = rank::EIGHT;
	int f = file::A;
	while (c = *p++) {
		if (c == '/') {
			r--;
			f = file::A;
			continue;
		}
		else if (isdigit(c)) {
			int num_empty = c - '0';
			f += num_empty;
			if (num_empty < 1 || num_empty > 8) {
				std::cerr << "AnkaError (LoadPosition): Error parsing position FEN.\n";
				return false;
			}
			continue;
		}
		else {
			Square sq = square::RankFileToSquare(r, f);
			f++;

			if (isupper(c))
				bitboard::SetBit(m_piecesBB[side::WHITE], sq);
			else if (islower(c))
				bitboard::SetBit(m_piecesBB[side::BLACK], sq);
			else {
				std::cerr << "AnkaError (LoadPosition): Error parsing position FEN.\n";
				return false;
			}

			switch (c) {
			case 'p':
			case 'P':
				m_total_material += MATERIAL_VALUES[piece_type::PAWN];
				bitboard::SetBit(m_piecesBB[piece_type::PAWN], sq);
				m_board[sq] = piece_type::PAWN;
				break;
			case 'n':
			case 'N':
				m_total_material += MATERIAL_VALUES[piece_type::KNIGHT];
				bitboard::SetBit(m_piecesBB[piece_type::KNIGHT], sq);
				m_board[sq] = piece_type::KNIGHT;
				break;
			case 'b':
			case 'B':
				m_total_material += MATERIAL_VALUES[piece_type::BISHOP];
				bitboard::SetBit(m_piecesBB[piece_type::BISHOP], sq);
				m_board[sq] = piece_type::BISHOP;
				break;
			case 'r':
			case 'R':
				m_total_material += MATERIAL_VALUES[piece_type::ROOK];
				bitboard::SetBit(m_piecesBB[piece_type::ROOK], sq);
				m_board[sq] = piece_type::ROOK;
				break;
			case 'q':
			case 'Q':
				m_total_material += MATERIAL_VALUES[piece_type::QUEEN];
				bitboard::SetBit(m_piecesBB[piece_type::QUEEN], sq);
				m_board[sq] = piece_type::QUEEN;
				break;
			case 'k':
			case 'K':
				bitboard::SetBit(m_piecesBB[piece_type::KING], sq);
				m_board[sq] = piece_type::KING;
				break;
			default:
				std::cerr << "AnkaError (LoadPosition): Error parsing position FEN.\n";
				return false;

			}
		}
	}

	m_occupation = m_piecesBB[side::WHITE] | m_piecesBB[side::BLACK];
	m_zobrist_key = CalculateKey();

	ANKA_ASSERT(Validate());

	return true;
}


void anka::GameState::Print()
{
	putchar('\n');
	char board[8][8];
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			board[i][j] = '.';
		}
	}
	constexpr char piece_chars[] = { 'P', 'N', 'B', 'R', 'Q', 'K' };

	Bitboard white_pieces = Pieces<side::WHITE, piece_type::LOWERBOUND>();
	for (int i = 2; i < 8; i++) {
		Bitboard pieces = m_piecesBB[i];
		char piece_char = piece_chars[i - 2];

		while (pieces) {
			Square sq = bitboard::BitScanForward(pieces);
			Rank rank = square::GetRank(sq);
			File file = square::GetFile(sq);
			if (bitboard::BitIsSet(white_pieces, sq)) {
				board[rank][file] = piece_char;
			}
			else {
				board[rank][file] = tolower(piece_char);
			}

			pieces &= pieces - 1;
		} 
	}

	
	for (int r = rank::EIGHT; r >= rank::ONE; r--) {
		printf(" %d|  ", r + 1);
		for (int f = file::A; f <= file::H; f++) {
			printf("%c ", board[r][f]);
		}
		putchar('\t');
		if (r == rank::EIGHT) {
			printf("Side to move: %s\n", side::ToString(SideToPlay()));
		}
		else if (r == rank::SEVEN) {
			printf("Half-Move Clock: %d\n", HalfMoveClock());
		}
		else if (r == rank::SIX) {
			printf("Castling Rights: %s\n", CastlingRightsToString(m_castling_rights));
		}
		else if (r == rank::FIVE) {
			printf("En passant target: %s\n", square::ToString(EnPassantSquare()));
		}
		else if (r == rank::FOUR) {
			printf("Zobrist key: 0x%" PRIx64 "\n", PositionKey());
		}
		else {
			putchar('\n');
		}
	}
	printf("     _______________ \n");
	printf("     a b c d e f g h \n\n");
}

void anka::GameState::PrintBitboards()
{
	printf("***White pieces***\n");
	bitboard::Print(Pieces<side::WHITE>());
	putchar('\n');

	printf("***Black pieces***\n");
	bitboard::Print(Pieces<side::BLACK>());
	putchar('\n');

	printf("***Pawns***\n");
	bitboard::Print(Pieces<side::LOWERBOUND, piece_type::PAWN>());
	putchar('\n');

	printf("***Knights***\n");
	bitboard::Print(Pieces<side::LOWERBOUND, piece_type::KNIGHT>());
	putchar('\n');

	printf("***Bishops***\n");
	bitboard::Print(Pieces<side::LOWERBOUND, piece_type::BISHOP>());
	putchar('\n');

	printf("***Rooks***\n");
	bitboard::Print(Pieces<side::LOWERBOUND, piece_type::ROOK>());
	putchar('\n');

	printf("***Queens***\n");
	bitboard::Print(Pieces<side::LOWERBOUND, piece_type::QUEEN>());
	putchar('\n');

	printf("***Kings***\n");
	bitboard::Print(Pieces<side::LOWERBOUND, piece_type::KING>());
	putchar('\n');
}