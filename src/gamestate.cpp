#include "gamestate.hpp"
#include "movegen.hpp"
#include <sstream>
#include <inttypes.h>
#include <ctype.h>
#include <string.h>
#include <iostream>
#include <assert.h>


void anka::GameState::Clear()
{
	for (int i = 0; i < 8; i++) {
		m_piecesBB[i] = C64(0);
	}

	for (int sq = 0; sq < 64; sq++) {
		m_board[sq] = NO_PIECE;
	}
	m_occupation = C64(0);

	m_ep_target = NO_SQUARE;
	m_castling_rights = 0;
	m_halfmove_clock = 0;
	m_side = NOSIDE;
	m_zobrist_key = C64(0);
	m_root_ply = 0;
}

// BUG: trailing whitespaces in fens cause a crash
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
		if (num_parts >= 6 || fen_parts[num_parts].length() == 0)
			break;
		if (fen_parts[num_parts].back() == '\n')
			fen_parts[num_parts].erase(fen_parts[num_parts].length() - 1);
		++num_parts;
	}

	// Side to move
	if (fen_parts[1] == "w") {
		m_side = WHITE;
	}
	else if (fen_parts[1] == "b") {
		m_side = BLACK;
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
		if (f > FILE_H|| f < FILE_A|| r > RANK_EIGHT|| r < RANK_ONE) {
			std::cerr << "AnkaError (LoadPosition): Error parsing en passant square.\n";
			return false;
		}
		m_ep_target = RankFileToSquare(r, f);
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
	int r = RANK_EIGHT;
	int f = FILE_A;
	while (c = *p++) {
		if (c == '/') {
			r--;
			f = FILE_A;
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
			Square sq = RankFileToSquare(r, f);
			f++;

			PieceType piece = CharToPieceType(c);
			if (piece == NO_PIECE) {
				std::cerr << "AnkaError (LoadPosition): Error parsing position FEN.\n";
				return false;
			}

			bitboard::SetBit(m_piecesBB[piece], sq);
			m_board[sq] = piece;


			if (isupper(c)) {
				bitboard::SetBit(m_piecesBB[WHITE], sq);
			}
			else if (islower(c)) {
				bitboard::SetBit(m_piecesBB[BLACK], sq);
			}
			else {
				std::cerr << "AnkaError (LoadPosition): Error parsing position FEN.\n";
				return false;
			}
		}
	}

	m_occupation = m_piecesBB[WHITE] | m_piecesBB[BLACK];
	m_zobrist_key = CalculateKey();

	ANKA_ASSERT(Validate());

	return true;
}


// Create a move from the string and make sure it is a valid move in the position
anka::Move anka::GameState::ParseMove(const char* line) const
{
	char move_str[6];
	//strcpy(move_str, line);
	strncpy(move_str, line, 5);
	move_str[5] = '\0';
	auto move_str_len = strlen(move_str);
	if (move_str_len < 4 || move_str_len > 5)
		return 0;

	if (move_str[0] > 'h' || move_str[0] < 'a')
		return 0;

	if (move_str[1] > '8' || move_str[1] < '1')
		return 0;

	if (move_str[2] > 'h' || move_str[2] < 'a')
		return 0;

	if (move_str[3] > '8' || move_str[3] < '1')
		return 0;

	Square from = RankFileToSquare(move_str[1] - '1', move_str[0] - 'a');
	Square to = RankFileToSquare(move_str[3] - '1', move_str[2] - 'a');

	ANKA_ASSERT(from >= 0 && from < 64);
	ANKA_ASSERT(to >= 0 && to < 64);

	MoveList<256> list;
	list.GenerateLegalMoves(*this);

	// find the matching move from possible moves and return it
	for (int i = 0; i < list.length; i++) {
		Move move = list.moves[i].move;
		if (move::FromSquare(move) == from && move::ToSquare(move) == to) {
			if (move::IsPromotion(move)) {
				if (move_str_len != 5)
					return 0;

				PieceType promoted = move::PromotedPiece(move);
				if (promoted == ROOK && move_str[4] == 'r') {
					return move;
				}
				else if (promoted == BISHOP && move_str[4] == 'b') {
					return move;
				}
				else if (promoted == QUEEN && move_str[4] == 'q') {
					return move;
				}
				else if (promoted == KNIGHT && move_str[4] == 'n') {
					return move;
				}
			}
			else {
				return move;
			}
		}
	}

	return 0;
}

void anka::GameState::Print() const
{
	putchar('\n');
	char board[8][8];
	for (int i = 0; i < 8; i++) {
		for (int j = 0; j < 8; j++) {
			board[i][j] = '.';
		}
	}
	constexpr char piece_chars[] = { 'P', 'N', 'B', 'R', 'Q', 'K' };

	Bitboard white_pieces = Pieces<WHITE, ALL_PIECES>();
	for (int i = 2; i < 8; i++) {
		Bitboard pieces = m_piecesBB[i];
		char piece_char = piece_chars[i - 2];

		while (pieces) {
			Square sq = bitboard::BitScanForward(pieces);
			Rank rank = GetRank(sq);
			File file = GetFile(sq);
			if (bitboard::BitIsSet(white_pieces, sq)) {
				board[rank][file] = piece_char;
			}
			else {
				board[rank][file] = tolower(piece_char);
			}

			pieces &= pieces - 1;
		} 
	}

	
	for (int r = RANK_EIGHT; r >= RANK_ONE; r--) {
		printf(" %d|  ", r + 1);
		for (int f = FILE_A; f <= FILE_H; f++) {
			printf("%c ", board[r][f]);
		}
		putchar('\t');
		if (r == RANK_EIGHT) {
			printf("Side to move: %s\n", SideToString(SideToPlay()));
		}
		else if (r == RANK_SEVEN) {
			printf("Half-Move Clock: %d\n", HalfMoveClock());
		}
		else if (r == RANK_SIX) {
			printf("Castling Rights: %s\n", CastlingRightsToString(m_castling_rights));
		}
		else if (r == RANK_FIVE) {
			printf("En passant target: %s\n", SquareToString(EnPassantSquare()));
		}
		else if (r == RANK_FOUR) {
			printf("Zobrist key: 0x%" PRIx64 "\n", PositionKey());
		}
		else {
			putchar('\n');
		}
	}
	printf("     _______________ \n");
	printf("     a b c d e f g h \n\n");
}

void anka::GameState::PrintBitboards() const
{
	printf("***White pieces***\n");
	bitboard::PrintBitboard(Pieces<WHITE>());
	putchar('\n');

	printf("***Black pieces***\n");
	bitboard::PrintBitboard(Pieces<BLACK>());
	putchar('\n');

	printf("***Pawns***\n");
	bitboard::PrintBitboard(Pieces<ALLSIDES, PAWN>());
	putchar('\n');

	printf("***Knights***\n");
	bitboard::PrintBitboard(Pieces<ALLSIDES, KNIGHT>());
	putchar('\n');

	printf("***Bishops***\n");
	bitboard::PrintBitboard(Pieces<ALLSIDES, BISHOP>());
	putchar('\n');

	printf("***Rooks***\n");
	bitboard::PrintBitboard(Pieces<ALLSIDES, ROOK>());
	putchar('\n');

	printf("***Queens***\n");
	bitboard::PrintBitboard(Pieces<ALLSIDES, QUEEN>());
	putchar('\n');

	printf("***Kings***\n");
	bitboard::PrintBitboard(Pieces<ALLSIDES, KING>());
	putchar('\n');
}
