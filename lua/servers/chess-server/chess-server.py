import asyncio
from aiohttp import web, WSMsgType
import aiohttp_cors
import json
import uuid
import time
import chess
import argparse
import ssl
import os
from typing import Dict, List, Any, Optional

# --- 1. Global Server State (In-Memory Single Source of Truth) ---
# Maps userId to the active WebSocket connection object
CONNECTIONS: Dict[str, web.WebSocketResponse] = {}
# FIFO queue of users waiting for a match
MATCH_QUEUE: List[str] = []
# Maps gameId to the current GameState object
ACTIVE_GAMES: Dict[str, Any] = {}

# --- 2. Configuration and Flags ---
# Set to True to enable database interaction (currently OFF as requested)
DB_ENABLED = False
DEFAULT_PORT = 8081
INFINITE_TIME_MS = 999999999  # Use a large integer instead of float('inf')

# --- 3. Utility Functions ---


def generate_id() -> str:
    """Generates a unique ID for games."""
    return str(uuid.uuid4())


async def send_message(ws: web.WebSocketResponse, message: Dict[str, Any]):
    """Safely sends a structured JSON message over a websocket."""
    try:
        await ws.send_str(json.dumps(message))
    except Exception:
        # Note: If the connection is closed, this will fail silently on send_str
        pass


def get_game_state_for_send(game_state: Dict[str, Any]) -> Dict[str, Any]:
    """Prepares a GameState object for sending to the client."""

    # Replace infinite time with a large integer for JSON compatibility
    def safe_time(val):
        if val is None:
            return INFINITE_TIME_MS
        if isinstance(val, float) and (val == float('inf') or val != val):
            return INFINITE_TIME_MS
        return int(val)

    return {
        "gameId": game_state["gameId"],
        "playerWhiteId": game_state["playerWhiteId"],
        "playerBlackId": game_state["playerBlackId"],
        "fen": game_state["boardState"].fen(),
        "currentTurn": game_state["currentTurn"],
        "status": game_state["status"],
        "timeControlPreset": game_state["timeControlPreset"],
        "timeWhiteRemainingMs": safe_time(game_state.get("timeWhiteRemainingMs")),
        "timeBlackRemainingMs": safe_time(game_state.get("timeBlackRemainingMs")),
    }


# --- 4. Game Initialization and Matching Logic ---
async def create_new_game(p_white_id: str, p_black_id: str):
    """Initializes a new game state and notifies both players."""
    game_id = generate_id()

    # Initialize the chess board using the python-chess library
    new_board = chess.Board()

    # Game State Structure
    game_state = {
        "gameId": game_id,
        "playerWhiteId": p_white_id,
        "playerBlackId": p_black_id,
        "boardState": new_board,  # The actual chess object
        "fen": new_board.fen(),
        "currentTurn": "white",
        "status": "IN_PROGRESS",
        "timeControlPreset": "none",
        "timeWhiteRemainingMs": INFINITE_TIME_MS,
        "timeBlackRemainingMs": INFINITE_TIME_MS,
        "lastMoveTimestamp": time.time(),
    }

    ACTIVE_GAMES[game_id] = game_state

    # Send MATCH_FOUND to White
    await send_message(
        CONNECTIONS[p_white_id],
        {
            "type": "MATCH_FOUND",
            "gameId": game_id,
            "color": "white",
            "opponentId": p_black_id,
            **get_game_state_for_send(game_state),
        },
    )

    # Send MATCH_FOUND to Black
    await send_message(
        CONNECTIONS[p_black_id],
        {
            "type": "MATCH_FOUND",
            "gameId": game_id,
            "color": "black",
            "opponentId": p_white_id,
            **get_game_state_for_send(game_state),
        },
    )

    print(f"Game {game_id} started: {p_white_id} (W) vs {p_black_id} (B)")

    # DB Persistence Placeholder
    if DB_ENABLED:
        pass


async def join_queue_handler(user_id: str):
    """Handles players joining the queue and initiates matching."""
    if user_id in MATCH_QUEUE:
        return

    MATCH_QUEUE.append(user_id)
    print(f"Player {user_id} joined queue. Queue size: {len(MATCH_QUEUE)}")

    # Matching Check
    if len(MATCH_QUEUE) >= 2:
        p1_id = MATCH_QUEUE.pop(0)
        p2_id = MATCH_QUEUE.pop(0)
        await create_new_game(p1_id, p2_id)


# --- 5. Game Logic ---


async def make_move_handler(user_id: str, payload: Dict[str, Any]):
    """Handles a move attempt, performs validation, and broadcasts updates."""
    game_id = str(payload.get("gameId"))
    move_uci = payload.get("move")  # Move in UCI format (e.g., 'e2e4')

    game_state = ACTIVE_GAMES.get(game_id)

    # Validation 1 (Game State)
    if not game_state or game_state["status"] != "IN_PROGRESS":
        print(f"Move rejected for {user_id}: Game {game_id} not active.")
        return

    # Validation 2 (Turn)
    is_white = user_id == game_state["playerWhiteId"]
    is_black = user_id == game_state["playerBlackId"]

    expected_turn = "white" if game_state["boardState"].turn == chess.WHITE else "black"

    if (expected_turn == "white" and not is_white) or (expected_turn == "black" and not is_black):
        await send_message(
            CONNECTIONS[user_id],
            {"type": "MOVE_REJECTED", "gameId": game_id, "reason": "Not your turn.", "fen": game_state["fen"]},
        )
        return

    # Validation 3 (Legal Move)
    board: chess.Board = game_state["boardState"]

    if not isinstance(move_uci, str):
        await send_message(
            CONNECTIONS[user_id],
            {"type": "MOVE_REJECTED", "gameId": game_id, "reason": "Invalid move format.", "fen": game_state["fen"]},
        )
        return

    try:
        # Try to parse and push the move
        move = board.parse_uci(move_uci)
        if move not in board.legal_moves:
            raise ValueError("Move not in legal moves set.")

        board.push(move)

    except (ValueError, IndexError):
        await send_message(
            CONNECTIONS[user_id],
            {"type": "MOVE_REJECTED", "gameId": game_id, "reason": "Illegal move.", "fen": game_state["fen"]},
        )
        return

    # --- Move Successful ---

    game_state["lastMoveTimestamp"] = time.time()

    # Check for terminal conditions
    game_end_message: Optional[Dict[str, Any]] = None
    winner_id = None
    reason = None

    if board.is_checkmate():
        winner_id = game_state["playerBlackId"] if board.turn == chess.WHITE else game_state["playerWhiteId"]
        reason = "Checkmate"
        game_state["status"] = "WHITE_WINS" if winner_id == game_state["playerWhiteId"] else "BLACK_WINS"
    elif (
        board.is_stalemate()
        or board.is_insufficient_material()
        or board.is_fivefold_repetition()
        or board.is_seventyfive_moves()
    ):
        winner_id = None
        reason = "Draw"
        game_state["status"] = "DRAW"

    # Set up game end message if applicable
    if game_state["status"] != "IN_PROGRESS":
        game_end_message = {
            "type": "GAME_END",
            "gameId": game_id,
            "result": "DRAW" if game_state["status"] == "DRAW" else "WIN",
            "winnerId": winner_id,
            "reason": reason,
        }
        # Clear game state from memory after game end
        del ACTIVE_GAMES[game_id]
        print(f"Game {game_id} finished: {reason}. Status: {game_state['status']}")

    # Update turn and FEN string
    game_state["currentTurn"] = "white" if board.turn == chess.WHITE else "black"
    game_state["fen"] = board.fen()

    # Create GAME_UPDATE message
    update_message = {
        "type": "GAME_UPDATE",
        "gameId": game_id,
        "lastMove": move_uci,
        **get_game_state_for_send(game_state),
    }

    # Broadcast to both players
    p_white_socket = CONNECTIONS.get(game_state["playerWhiteId"])
    p_black_socket = CONNECTIONS.get(game_state["playerBlackId"])

    if p_white_socket:
        await send_message(p_white_socket, update_message)
    if p_black_socket:
        await send_message(p_black_socket, update_message)

    # Send GAME_END if game finished
    if game_end_message:
        if p_white_socket:
            await send_message(p_white_socket, game_end_message)
        if p_black_socket:
            await send_message(p_black_socket, game_end_message)

    # DB Persistence Placeholder
    if DB_ENABLED:
        pass


# --- 6. Disconnection Handling ---


async def handle_disconnection(user_id: str):
    """Treats disconnection as resignation and ends the game."""
    print(f"Player {user_id} disconnected.")

    # 1. Remove from in-memory connection list
    CONNECTIONS.pop(user_id, None)

    # 2. Remove from queue if waiting
    if user_id in MATCH_QUEUE:
        MATCH_QUEUE.remove(user_id)
        print(f"Removed {user_id} from queue.")
        return

    # 3. Search active games
    games_to_end = [
        g_id
        for g_id, state in ACTIVE_GAMES.items()
        if state["playerWhiteId"] == user_id or state["playerBlackId"] == user_id
    ]

    for game_id in games_to_end:
        game_state = ACTIVE_GAMES.get(game_id)
        if not game_state or game_state["status"] != "IN_PROGRESS":
            continue

        # Determine winner
        resigning_color = "white" if game_state["playerWhiteId"] == user_id else "black"
        winner_id = game_state["playerBlackId"] if resigning_color == "white" else game_state["playerWhiteId"]

        # Update state
        game_state["status"] = "OPPONENT_RESIGNED"

        # Notify the opponent
        opponent_socket = CONNECTIONS.get(winner_id)
        if opponent_socket:
            # Notify opponent of disconnection
            await send_message(opponent_socket, {"type": "OPPONENT_DISCONNECTED", "gameId": game_id})

            # Send GAME_END
            await send_message(
                opponent_socket,
                {
                    "type": "GAME_END",
                    "gameId": game_id,
                    "result": "WIN",
                    "winnerId": winner_id,
                    "reason": f"Opponent ({resigning_color}) disconnected/resigned.",
                },
            )

        print(f"Game {game_id} ended due to resignation by {user_id}. Winner: {winner_id}")

        # Persistence and cleanup
        if DB_ENABLED:
            pass

        ACTIVE_GAMES.pop(game_id, None)  # Use .pop for safety


# --- 7. WebSocket Handler (Aiohttp specific) ---


async def ws_handler(request):
    """Handles new connections and message routing."""
    ws = web.WebSocketResponse()
    await ws.prepare(request)

    user_id = None
    connection_id = generate_id()
    print(f"New connection: {connection_id}")

    try:
        # Phase 1: Authentication (Mandatory initial message)
        try:
            # Use a slightly longer timeout for the first message
            msg = await asyncio.wait_for(ws.receive(), timeout=10.0)
        except asyncio.TimeoutError:
            print(f"Connection {connection_id} timed out waiting for AUTH.")
            return ws

        if msg.type != WSMsgType.TEXT:
            await ws.close()
            return ws

        data = json.loads(msg.data)
        if data.get("type") != "AUTH" or "userId" not in data:
            print(f"Connection {connection_id} closed: Missing or invalid AUTH message.")
            await ws.close()
            return ws

        user_id = data["userId"]
        CONNECTIONS[user_id] = ws
        print(f"Player {user_id} authenticated. Total active connections: {len(CONNECTIONS)}")

        # Phase 2: Message Loop
        async for msg in ws:
            if msg.type == WSMsgType.TEXT:
                try:
                    data = json.loads(msg.data)
                    msg_type = data.get("type")

                    if msg_type == "JOIN_QUEUE":
                        await join_queue_handler(user_id)
                    elif msg_type == "MAKE_MOVE":
                        await make_move_handler(user_id, data)
                    elif msg_type == "RESIGN":
                        await handle_disconnection(user_id)
                    elif msg_type == "PING":
                        await send_message(ws, {"type": "PONG"})
                    else:
                        print(f"Unknown message type from {user_id}: {msg_type}")
                except json.JSONDecodeError:
                    print(f"Received invalid JSON from {user_id}")
                except Exception as e:
                    print(f"Error processing message from {user_id}: {e}")
            elif msg.type == WSMsgType.ERROR:
                print(f"WebSocket connection closed with exception {ws.exception()}")
                break
            # Handle close messages explicitly
            elif msg.type in (WSMsgType.CLOSE, WSMsgType.CLOSED):
                break

    except Exception as e:
        # Catch unexpected errors during connection lifetime
        print(f"Unexpected connection error for {connection_id}: {e}")
    finally:
        # Phase 3: Disconnection Cleanup
        if user_id:
            # Note: We don't await handle_disconnection here to prevent blocking,
            # but in this synchronous structure, it's safer to ensure cleanup runs.
            await handle_disconnection(user_id)
        # Close the socket if it hasn't been closed already
        if not ws.closed:
            await ws.close()

    return ws


# --- 8. HTTP Handler to Serve Frontend and Static Files ---


async def index_handler(request):
    """
    Serves the static index.html file from the ./static directory.
    """
    static_dir = os.path.join(os.path.dirname(__file__), "static")
    file_path = os.path.join(static_dir, "index.html")
    if not os.path.isfile(file_path):
        raise web.HTTPNotFound()
    return web.FileResponse(file_path)


async def static_file_handler(request):
    """
    Serves static files from the ./static directory, supporting subdirectories.
    Example: /assets/foo.png -> ./static/assets/foo.png
    """
    rel_path = request.match_info.get('filename')
    # Prevent directory traversal
    safe_path = os.path.normpath(rel_path).replace("\\", "/")
    if safe_path.startswith(".."):
        raise web.HTTPForbidden()
    static_dir = os.path.join(os.path.dirname(__file__), "static")
    file_path = os.path.join(static_dir, safe_path)
    if not os.path.isfile(file_path):
        raise web.HTTPNotFound()
    return web.FileResponse(file_path)


# --- 9. Server Startup and Configuration ---


def configure_app():
    """Configures the aiohttp application with routes and CORS."""
    app = web.Application()

    # Setup routes
    app.router.add_get('/', index_handler, name='index')
    app.router.add_get('/ws', ws_handler, name='websocket')
    # Serve static files from /static and subdirectories
    app.router.add_get('/{filename:.*}', static_file_handler, name='static-file')

    # Configure CORS for the application (allowing all origins for development)
    cors = aiohttp_cors.setup(
        app,
        defaults={
            "*": aiohttp_cors.ResourceOptions(
                allow_credentials=True, expose_headers="*", allow_headers="*", allow_methods=["GET", "POST"]
            )
        },
    )

    # Apply CORS to all routes
    for route in list(app.router.routes()):
        if route.name:  # Only apply to named routes
            cors.add(route)

    return app


def parse_args():
    """Parses command-line arguments for SSL configuration."""
    parser = argparse.ArgumentParser(description="Aiohttp Real-Time Chess Server")
    parser.add_argument(
        '--cert', type=str, default=None, help='Path to the SSL certificate file (e.g., fullchain.pem).'
    )
    parser.add_argument('--key', type=str, default=None, help='Path to the SSL private key file (e.g., privkey.pem).')
    parser.add_argument(
        '--port', type=int, default=DEFAULT_PORT, help=f'Port to run the server on (default: {DEFAULT_PORT}).'
    )
    return parser.parse_args()


async def start_server():
    """Initializes and starts the aiohttp server."""
    args = parse_args()
    app = configure_app()
    ssl_context = None
    host = "0.0.0.0"

    # --- SSL Configuration ---
    if args.cert and args.key:
        if not os.path.exists(args.cert) or not os.path.exists(args.key):
            print("ERROR: SSL files not found. Running insecurely (HTTP/WS).")
        else:
            ssl_context = ssl.create_default_context(ssl.Purpose.CLIENT_AUTH)
            try:
                ssl_context.load_cert_chain(args.cert, args.key)
                print(f"Server will run securely (HTTPS/WSS) on port {args.port}")
            except Exception as e:
                print(f"Error loading SSL certificate chain: {e}. Running insecurely (HTTP/WS).")
                ssl_context = None

    if ssl_context is None:
        print(f"Server will run insecurely (HTTP/WS) on port {args.port}")

    # --- DB Status Message ---
    if DB_ENABLED:
        print("Database functionality is ENABLED (but currently empty placeholders).")
    else:
        print("Database functionality is DISABLED. Using in-memory state only.")

    # --- Start the App ---
    runner = web.AppRunner(app)
    await runner.setup()

    # Use the provided SSL context if available
    site = web.TCPSite(runner, host, args.port, ssl_context=ssl_context)
    await site.start()

    protocol_name = "https" if ssl_context else "http"
    ws_protocol_name = "wss" if ssl_context else "ws"
    print("-" * 50)
    print(f"🌐 Frontend URL: {protocol_name}://localhost:{args.port}/")
    print(f"🔌 WebSocket URL: {ws_protocol_name}://localhost:{args.port}/ws")
    print("-" * 50)

    # Keep the server running forever
    await asyncio.Future()


if __name__ == "__main__":
    try:
        asyncio.run(start_server())
    except KeyboardInterrupt:
        print("\nServer stopped manually.")
