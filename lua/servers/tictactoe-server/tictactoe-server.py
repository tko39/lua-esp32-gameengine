"""
WebSocket and HTTP server for Tic-Tac-Toe game
"""

import asyncio
import json
import ssl
import logging
from pathlib import Path
from typing import Optional

from aiohttp import web, WSMsgType
import aiohttp_cors

from room import RoomManager, RoomStatus
from game import CellState, GameResult


logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


class TicTacToeServer:
    """WebSocket and HTTP server for Tic-Tac-Toe"""

    def __init__(self, host='0.0.0.0', port=8080, ssl_context: Optional[ssl.SSLContext] = None):
        self.host = host
        self.port = port
        self.ssl_context = ssl_context
        self.app = web.Application()
        self.room_manager = RoomManager()

        # Setup routes
        self.app.router.add_get('/ws', self.websocket_handler)
        self.app.router.add_get('/', self.http_index)
        self.app.router.add_get('/health', self.health_check)

        # Setup CORS
        cors = aiohttp_cors.setup(
            self.app,
            defaults={
                "*": aiohttp_cors.ResourceOptions(
                    allow_credentials=True,
                    expose_headers="*",
                    allow_headers="*",
                )
            },
        )

        # Configure CORS on all routes
        for route in list(self.app.router.routes()):
            if not isinstance(route.resource, web.StaticResource):
                cors.add(route)

        # Serve static files for browser client
        static_dir = Path(__file__).parent / 'static'
        if static_dir.exists():
            self.app.router.add_static('/static/', path=static_dir, name='static')

    async def health_check(self, request):
        """Health check endpoint"""
        return web.json_response(
            {
                'status': 'ok',
                'active_rooms': len(self.room_manager.rooms),
                'waiting_rooms': 1 if self.room_manager.waiting_room else 0,
            }
        )

    async def http_index(self, request):
        """Serve HTML client"""
        static_dir = Path(__file__).parent / 'static'
        index_file = static_dir / 'index.html'

        if index_file.exists():
            return web.FileResponse(index_file)
        else:
            return web.Response(
                text="""
                <html>
                <head><title>Tic-Tac-Toe Server</title></head>
                <body>
                    <h1>Tic-Tac-Toe WebSocket Server</h1>
                    <p>WebSocket endpoint: ws://localhost:8080/ws</p>
                    <p>Active rooms: {}</p>
                </body>
                </html>
            """.format(
                    len(self.room_manager.rooms)
                ),
                content_type='text/html',
            )

    async def send_message(self, ws, msg_type: str, data: dict):
        """Send JSON message to WebSocket client"""
        message = {'type': msg_type, **data}
        await ws.send_json(message)
        logger.info(f"Sent {msg_type}: {data}")

    async def websocket_handler(self, request):
        """Handle WebSocket connections"""
        ws = web.WebSocketResponse()
        await ws.prepare(request)

        logger.info("New WebSocket connection")
        room = None

        try:
            async for msg in ws:
                if msg.type == WSMsgType.TEXT:
                    try:
                        data = json.loads(msg.data)
                        msg_type = data.get('type')

                        if msg_type == 'handshake':
                            await self.handle_handshake(ws, data)

                        elif msg_type == 'move':
                            await self.handle_move(ws, data)

                        else:
                            await self.send_message(
                                ws, 'error', {'message': f'Unknown message type: {msg_type}', 'code': 'UNKNOWN_TYPE'}
                            )

                    except json.JSONDecodeError:
                        await self.send_message(ws, 'error', {'message': 'Invalid JSON', 'code': 'INVALID_JSON'})
                    except Exception as e:
                        logger.error(f"Error handling message: {e}", exc_info=True)
                        await self.send_message(ws, 'error', {'message': str(e), 'code': 'SERVER_ERROR'})

                elif msg.type == WSMsgType.ERROR:
                    logger.error(f'WebSocket error: {ws.exception()}')

        finally:
            # Clean up on disconnect
            logger.info("WebSocket disconnected")
            await self.handle_disconnect(ws)

        return ws

    async def handle_disconnect(self, ws):
        """Handle player disconnection"""
        old_room, opponent = self.room_manager.remove_player(ws)

        if not old_room:
            return

        # If opponent exists, they get a win and are moved to a new room
        if opponent:
            logger.info(f"Player disconnected from room {old_room.room_id}, opponent gets win")

            # Create new room for the remaining player
            new_room = self.room_manager.create_room()
            new_player, new_symbol = new_room.add_player(opponent.ws)

            # Update player room mapping
            self.room_manager.player_rooms[opponent.ws] = new_room.room_id

            # Check if we can pair with waiting room
            if self.room_manager.waiting_room is None:
                self.room_manager.waiting_room = new_room
                new_status = RoomStatus.WAITING
            elif self.room_manager.waiting_room.room_id != new_room.room_id:
                # Try to join waiting room if it exists
                waiting = self.room_manager.waiting_room
                if not waiting.is_full():
                    # Move player to waiting room
                    new_room.remove_player(opponent.ws)
                    del self.room_manager.rooms[new_room.room_id]
                    new_room = waiting
                    new_player, new_symbol = new_room.add_player(opponent.ws)
                    self.room_manager.player_rooms[opponent.ws] = new_room.room_id
                    if new_room.is_full():
                        self.room_manager.waiting_room = None
                        new_status = RoomStatus.PAIRED
                    else:
                        new_status = RoomStatus.WAITING
                else:
                    new_status = RoomStatus.WAITING
            else:
                new_status = RoomStatus.WAITING

            # Send game over message to remaining player (they win by forfeit)
            await self.send_message(
                opponent.ws,
                'game_over',
                {
                    'result': GameResult.WIN.value,
                    'winner': opponent.symbol.name,
                    'reason': 'opponent_disconnect',
                    'new_room_id': new_room.room_id,
                    'new_status': new_status.value,
                    'symbol': new_symbol.name,
                },
            )

            # Notify if the new room got paired
            if new_status == RoomStatus.PAIRED:
                other_opponent = new_room.get_opponent(opponent.ws)
                if other_opponent:
                    await self.send_message(other_opponent.ws, 'opponent_joined', {})
                    new_room.status = RoomStatus.PLAYING

            # Clean up old room
            old_room.status = RoomStatus.FINISHED
            if old_room.room_id in self.room_manager.rooms:
                del self.room_manager.rooms[old_room.room_id]
        else:
            logger.info(f"Player disconnected from empty room {old_room.room_id}")

    async def handle_handshake(self, ws, data):
        """Handle handshake and room assignment"""
        logger.info("Handling handshake")

        # Get or create room
        room, is_new = self.room_manager.get_or_create_room(ws)

        # Add player to room
        player, symbol = room.add_player(ws)

        # Send room assignment
        await self.send_message(
            ws, 'room_assigned', {'room_id': room.room_id, 'status': room.status.value, 'symbol': symbol.name}
        )

        # If room is now full, notify the other player
        if room.is_full():
            opponent = room.get_opponent(ws)
            if opponent:
                await self.send_message(opponent.ws, 'opponent_joined', {})
                room.status = RoomStatus.PLAYING

    async def handle_move(self, ws, data):
        """Handle player move"""
        position = data.get('position')

        if position is None or not isinstance(position, int):
            await self.send_message(ws, 'error', {'message': 'Invalid position', 'code': 'INVALID_POSITION'})
            return

        # Get room and player
        room = self.room_manager.get_room_by_ws(ws)
        if not room:
            await self.send_message(ws, 'error', {'message': 'Not in a room', 'code': 'NO_ROOM'})
            return

        player = room.get_player(ws)
        if not player:
            await self.send_message(ws, 'error', {'message': 'Player not found', 'code': 'NO_PLAYER'})
            return

        # Check if it's player's turn
        if room.game.current_turn != player.symbol:
            await self.send_message(ws, 'error', {'message': "Not your turn", 'code': 'NOT_YOUR_TURN'})
            return

        # Make the move
        if not room.game.make_move(position, player.symbol):
            await self.send_message(ws, 'error', {'message': 'Invalid move', 'code': 'INVALID_MOVE'})
            return

        # Notify opponent of the move
        opponent = room.get_opponent(ws)
        if opponent:
            await self.send_message(opponent.ws, 'move', {'position': position, 'player': player.symbol.name})

        # Check if game is over
        if room.game.is_game_over():
            await self.handle_game_over(room)

    async def handle_game_over(self, room):
        """Handle game over and create new rooms"""
        logger.info(f"Game over in room {room.room_id}")

        # Get results for each player
        player_x_result = room.game.get_game_result(CellState.X)
        player_o_result = room.game.get_game_result(CellState.O)

        winner = room.game.get_winner()
        winner_symbol = winner.name if winner else None

        # Create new rooms for each player
        new_rooms = {}

        for player in [room.player_x, room.player_o]:
            if player:
                # Create new room and add player
                new_room = self.room_manager.create_room()
                new_player, new_symbol = new_room.add_player(player.ws)

                # Update player room mapping
                self.room_manager.player_rooms[player.ws] = new_room.room_id

                # Check if we can pair with waiting room
                if self.room_manager.waiting_room is None:
                    self.room_manager.waiting_room = new_room
                    new_status = RoomStatus.WAITING
                elif self.room_manager.waiting_room.room_id != new_room.room_id:
                    # Try to join waiting room if it exists
                    waiting = self.room_manager.waiting_room
                    if not waiting.is_full():
                        # Move player to waiting room
                        new_room.remove_player(player.ws)
                        del self.room_manager.rooms[new_room.room_id]
                        new_room = waiting
                        new_player, new_symbol = new_room.add_player(player.ws)
                        self.room_manager.player_rooms[player.ws] = new_room.room_id
                        if new_room.is_full():
                            self.room_manager.waiting_room = None
                            new_status = RoomStatus.PAIRED
                        else:
                            new_status = RoomStatus.WAITING
                    else:
                        new_status = RoomStatus.WAITING
                else:
                    new_status = RoomStatus.WAITING

                new_rooms[player] = (new_room, new_status, new_symbol)

        # Send game over messages
        if room.player_x:
            new_room, new_status, new_symbol = new_rooms[room.player_x]
            await self.send_message(
                room.player_x.ws,
                'game_over',
                {
                    'result': player_x_result.value,
                    'winner': winner_symbol,
                    'new_room_id': new_room.room_id,
                    'new_status': new_status.value,
                    'symbol': new_symbol.name,
                },
            )

        if room.player_o:
            new_room, new_status, new_symbol = new_rooms[room.player_o]
            await self.send_message(
                room.player_o.ws,
                'game_over',
                {
                    'result': player_o_result.value,
                    'winner': winner_symbol,
                    'new_room_id': new_room.room_id,
                    'new_status': new_status.value,
                    'symbol': new_symbol.name,
                },
            )

        # Notify if rooms are paired
        for player, (new_room, new_status, new_symbol) in new_rooms.items():
            if new_status == RoomStatus.PAIRED:
                opponent = new_room.get_opponent(player.ws)
                if opponent:
                    await self.send_message(opponent.ws, 'opponent_joined', {})
                    new_room.status = RoomStatus.PLAYING

        # Mark old room as finished
        room.status = RoomStatus.FINISHED

        # Clean up old room
        if room.room_id in self.room_manager.rooms:
            del self.room_manager.rooms[room.room_id]

    def run(self):
        """Run the server"""
        logger.info(f"Starting server on {self.host}:{self.port}")
        logger.info(f"SSL: {'enabled' if self.ssl_context else 'disabled'}")

        web.run_app(self.app, host=self.host, port=self.port, ssl_context=self.ssl_context)


def create_ssl_context(cert_file: str, key_file: str) -> ssl.SSLContext:
    """Create SSL context for HTTPS/WSS"""
    ssl_context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
    ssl_context.load_cert_chain(cert_file, key_file)
    return ssl_context


def main():
    """Main entry point"""
    import argparse

    parser = argparse.ArgumentParser(description='Tic-Tac-Toe WebSocket Server')
    parser.add_argument('--host', default='0.0.0.0', help='Host to bind to')
    parser.add_argument('--port', type=int, default=8080, help='Port to bind to')
    parser.add_argument('--cert', help='SSL certificate file')
    parser.add_argument('--key', help='SSL key file')

    args = parser.parse_args()

    # Create SSL context if certificates provided
    ssl_context = None
    if args.cert and args.key:
        ssl_context = create_ssl_context(args.cert, args.key)

    # Create and run server
    server = TicTacToeServer(host=args.host, port=args.port, ssl_context=ssl_context)
    server.run()


if __name__ == '__main__':
    main()
