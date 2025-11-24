"""
Tic-Tac-Toe game logic
"""

from typing import Optional, List
from enum import Enum


class CellState(Enum):
    EMPTY = 0
    X = 1
    O = 2


class GameResult(Enum):
    WIN = "win"
    LOSS = "loss"
    DRAW = "draw"
    IN_PROGRESS = "in_progress"


class TicTacToeGame:
    """Tic-Tac-Toe game logic"""

    def __init__(self):
        self.board: List[CellState] = [CellState.EMPTY] * 9
        self.current_turn = CellState.X  # X always goes first

    def reset(self):
        """Reset the game board"""
        self.board = [CellState.EMPTY] * 9
        self.current_turn = CellState.X

    def is_valid_move(self, position: int) -> bool:
        """Check if a move is valid"""
        return 0 <= position < 9 and self.board[position] == CellState.EMPTY

    def make_move(self, position: int, player: CellState) -> bool:
        """Make a move on the board"""
        if not self.is_valid_move(position):
            return False

        if player != self.current_turn:
            return False

        self.board[position] = player
        self.current_turn = CellState.O if self.current_turn == CellState.X else CellState.X
        return True

    def check_line(self, a: int, b: int, c: int) -> bool:
        """Check if three positions form a winning line"""
        return self.board[a] != CellState.EMPTY and self.board[a] == self.board[b] == self.board[c]

    def get_winner(self) -> Optional[CellState]:
        """Get the winner of the game, if any"""
        # Check rows
        for i in range(3):
            if self.check_line(i * 3, i * 3 + 1, i * 3 + 2):
                return self.board[i * 3]

        # Check columns
        for i in range(3):
            if self.check_line(i, i + 3, i + 6):
                return self.board[i]

        # Check diagonals
        if self.check_line(0, 4, 8):
            return self.board[0]
        if self.check_line(2, 4, 6):
            return self.board[2]

        return None

    def is_board_full(self) -> bool:
        """Check if the board is full"""
        return all(cell != CellState.EMPTY for cell in self.board)

    def is_game_over(self) -> bool:
        """Check if the game is over"""
        return self.get_winner() is not None or self.is_board_full()

    def get_game_result(self, player: CellState) -> GameResult:
        """Get the game result for a specific player"""
        winner = self.get_winner()

        if winner is None:
            if self.is_board_full():
                return GameResult.DRAW
            return GameResult.IN_PROGRESS

        return GameResult.WIN if winner == player else GameResult.LOSS

    def get_board_state(self) -> List[str]:
        """Get board state as list of strings"""
        return [cell.name for cell in self.board]
