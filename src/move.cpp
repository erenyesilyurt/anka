#include "move.hpp"

int anka::move::ToString(Move move, char* result)
{
	int from_file = square::GetFile(FromSquare(move));
	int from_rank = square::GetRank(FromSquare(move));
	int to_file = square::GetFile(ToSquare(move));
	int to_rank = square::GetRank(ToSquare(move));
	int str_len = 4;
	PieceType promoted = piece_type::NOPIECE;
	if (IsPromotion(move)) {
		promoted = PromotedPiece(move);
	}

	result[0] = ('a' + from_file);
	result[1] = ('1' + from_rank);
	result[2] = ('a' + to_file);
	result[3] = ('1' + to_rank);
	result[4] = '\0';

	if (promoted != piece_type::NOPIECE) {
		char piece = 'q';
		if (promoted == piece_type::KNIGHT) {
			piece = 'n';
		}
		else if (promoted == piece_type::ROOK) {
			piece = 'r';
		}
		else if (promoted == piece_type::BISHOP) {
			piece = 'b';
		}

		result[4] = piece;
		result[5] = '\0';
		str_len = 5;
	}

	return str_len;
}