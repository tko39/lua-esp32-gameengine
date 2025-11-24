"""
Game room management
"""

import uuid
import asyncio
from typing import Optional, Dict
from enum import Enum
from game import TicTacToeGame, CellState, GameResult


class RoomStatus(Enum):
    WAITING = "waiting"
    PAIRED = "paired"
    PLAYING = "playing"
    FINISHED = "finished"


class Player:
    """Represents a player in a game room"""

    def __init__(self, ws, symbol: CellState):
        self.ws = ws
        self.symbol = symbol
        self.player_id = str(uuid.uuid4())[:8]


class GameRoom:
    """Represents a game room with two players"""

    def __init__(self, room_id: str):
        self.room_id = room_id
        self.game = TicTacToeGame()
        self.player_x: Optional[Player] = None
        self.player_o: Optional[Player] = None
        self.status = RoomStatus.WAITING

    def add_player(self, ws) -> tuple[Player, CellState]:
        """Add a player to the room and return their symbol"""
        if self.player_x is None:
            self.player_x = Player(ws, CellState.X)
            return self.player_x, CellState.X
        elif self.player_o is None:
            self.player_o = Player(ws, CellState.O)
            self.status = RoomStatus.PAIRED
            return self.player_o, CellState.O
        else:
            raise ValueError("Room is full")

    def remove_player(self, ws):
        """Remove a player from the room"""
        if self.player_x and self.player_x.ws == ws:
            self.player_x = None
        if self.player_o and self.player_o.ws == ws:
            self.player_o = None

        if self.player_x is None and self.player_o is None:
            self.status = RoomStatus.FINISHED
        elif self.player_x is None or self.player_o is None:
            self.status = RoomStatus.WAITING

    def get_player(self, ws) -> Optional[Player]:
        """Get player by websocket"""
        if self.player_x and self.player_x.ws == ws:
            return self.player_x
        if self.player_o and self.player_o.ws == ws:
            return self.player_o
        return None

    def get_opponent(self, ws) -> Optional[Player]:
        """Get opponent by websocket"""
        if self.player_x and self.player_x.ws == ws:
            return self.player_o
        if self.player_o and self.player_o.ws == ws:
            return self.player_x
        return None

    def is_full(self) -> bool:
        """Check if room has two players"""
        return self.player_x is not None and self.player_o is not None

    def is_empty(self) -> bool:
        """Check if room has no players"""
        return self.player_x is None and self.player_o is None


class RoomManager:
    """Manages all game rooms"""

    def __init__(self):
        self.rooms: Dict[str, GameRoom] = {}
        self.waiting_room: Optional[GameRoom] = None
        self.player_rooms: Dict[object, str] = {}  # ws -> room_id mapping

    def create_room(self) -> GameRoom:
        """Create a new game room"""
        room_id = str(uuid.uuid4())[:8]
        room = GameRoom(room_id)
        self.rooms[room_id] = room
        return room

    def get_or_create_room(self, ws) -> tuple[GameRoom, bool]:
        """Get waiting room or create new one. Returns (room, is_new)"""
        # Check if there's a waiting room
        if self.waiting_room and not self.waiting_room.is_full():
            room = self.waiting_room
            is_new = False
            # If this will fill the room, clear waiting_room
            if room.player_x is not None:
                self.waiting_room = None
        else:
            # Create new room
            room = self.create_room()
            self.waiting_room = room
            is_new = True

        # Map player to room
        self.player_rooms[ws] = room.room_id
        return room, is_new

    def get_room_by_ws(self, ws) -> Optional[GameRoom]:
        """Get room by player websocket"""
        room_id = self.player_rooms.get(ws)
        if room_id:
            return self.rooms.get(room_id)
        return None

    def remove_player(self, ws) -> tuple[Optional[GameRoom], Optional[Player]]:
        """Remove player from their room. Returns (room, opponent) if opponent exists."""
        room = self.get_room_by_ws(ws)
        if not room:
            return None, None

        # Get opponent before removing player
        opponent = room.get_opponent(ws)

        # Remove the player
        room.remove_player(ws)

        # Clean up empty rooms
        if room.is_empty():
            if room.room_id in self.rooms:
                del self.rooms[room.room_id]
            if self.waiting_room and self.waiting_room.room_id == room.room_id:
                self.waiting_room = None

        # Remove player mapping
        if ws in self.player_rooms:
            del self.player_rooms[ws]

        return room, opponent

    def cleanup_old_rooms(self):
        """Remove finished/abandoned rooms"""
        finished_rooms = [
            room_id for room_id, room in self.rooms.items() if room.status == RoomStatus.FINISHED or room.is_empty()
        ]
        for room_id in finished_rooms:
            del self.rooms[room_id]
