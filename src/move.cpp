#include "move.hpp"

int anka::move::ToString(Move move, char* result)
{
	int from_file = GetFile(FromSquare(move));
	int from_rank = GetRank(FromSquare(move));
	int to_file = GetFile(ToSquare(move));
	int to_rank = GetRank(ToSquare(move));
	int str_len = 4;
	PieceType promoted = NO_PIECE;
	if (IsPromotion(move)) {
		promoted = PromotedPiece(move);
	}

	result[0] = ('a' + from_file);
	result[1] = ('1' + from_rank);
	result[2] = ('a' + to_file);
	result[3] = ('1' + to_rank);
	result[4] = '\0';

	if (promoted != NO_PIECE) {
		char piece = 'q';
		if (promoted == KNIGHT) {
			piece = 'n';
		}
		else if (promoted == ROOK) {
			piece = 'r';
		}
		else if (promoted == BISHOP) {
			piece = 'b';
		}

		result[4] = piece;
		result[5] = '\0';
		str_len = 5;
	}

	return str_len;
}