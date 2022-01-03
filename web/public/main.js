// finish loading all WASM before calling chess functions
Module.onRuntimeInitialized = main

const canvas = document.getElementById("canvas")
const ctx = canvas.getContext("2d")
const width = canvas.width, height = canvas.height
const spaceWidth = canvas.width / 8, spaceHeight = canvas.height / 8

const gameStatusLabel = document.getElementById("gameStatus")
const gameHistoryArea = document.getElementById("gameHistory")
const movesConsideredLabel = document.getElementById("movesConsidered")
const undoBtn = document.getElementById("undo")
const resetBtn = document.getElementById("reset")
const engineLevelSelect = document.getElementById("engineLevel")
const playerColorSelect = document.getElementById("playerColor")
const chessOof = document.getElementById("chessOof")

var game
var engine
var playerColor = Colors.WHITE

var mousePosition = {x: 0, y: 0}
var pieceSelected = null
var pieceValidMoves = []
var moveHistory = []
var lastMoved = null

function main() {
    game = new ChessGame()
    engine = new ChessEngine(0)
    resetGame()

    resetBtn.onclick = resetGame

    undoBtn.onclick = function() {
        for (let i = 0; i < 2; i++) {
            if (moveHistory.length != 0) {
                game.undoMove()
                game.nextTurn()
                updateMoveHistory()
            } else break
        }
        pieceSelected = null
        pieceValidMoves = null
        lastMoved = null
        drawBoard()
    }

    engineLevelSelect.onchange = function() {
        engine.setLevel(parseInt(engineLevelSelect.value))
    }

    canvas.onmouseup = function(e) {
        let rect = canvas.getBoundingClientRect()
        mousePosition.x = e.clientX - rect.left
        mousePosition.y = e.clientY - rect.top
        const realPos = mouseToBoardPosition()
        if (pieceSelected == null) {
            pieceSelected = game.getPiece(realPos.x, realPos.y)
            if (pieceSelected != null && pieceSelected.color == playerColor && playerColor == game.getTurn()) {
                pieceValidMoves = game.getValidMoves(pieceSelected.x, pieceSelected.y)
            }
        } else {
            let found = pieceValidMoves.find(validMove => validMove.x == realPos.x && validMove.y == realPos.y)
            if (found != undefined) {
                handleMovePlayerPiece(realPos)
            }
            pieceSelected = null
            pieceValidMoves = null
        }
        drawBoard()
    }

    function handleMovePlayerPiece(nextPos) {
        game.movePiece(pieceSelected.x, pieceSelected.y, nextPos.x, nextPos.y)
        lastMoved = { x: pieceSelected.x, y: pieceSelected.y, x2: nextPos.x, y2: nextPos.y }
        game.nextTurn()
        drawBoard()
        updateGameStatus()
        updateMoveHistory()
        setTimeout(makeEnemyTurn, 250)
    }

    function makeEnemyTurn() {
        if (game.getTurn() != playerColor && !game.isCheckmate(game.getTurn()) && !game.isStalemate(game.getTurn())) {
            try {
                const movesInterval = setInterval(updateMovesConsidered, 250)
                const move = engine.generateMove(game, game.getTurn())
                game.movePiece(move.x, move.y, move.x2, move.y2)
                lastMoved = move
                game.nextTurn()
                updateGameStatus()
                clearInterval(movesInterval)
                updateMovesConsidered()
                updateMoveHistory()
                drawBoard()
            } catch (e) {
                updateGameStatus("Something went wrong! :(")
                console.log(e)
            }
        }

        function updateMovesConsidered() {
            movesConsideredLabel.textContent = `Moves Considered: ${engine.getNumberOfMovesConsidered()}`
        }
    }

    function showLastMoved() {
        if (lastMoved == null) return
        ctx.beginPath()
        ctx.strokeStyle = "#0059ff"
        ctx.lineWidth = 3
        const boardPos = upsidedownAdj({x: lastMoved.x, y: lastMoved.y})
        const boardPos2 = upsidedownAdj({x: lastMoved.x2, y: lastMoved.y2})
        ctx.strokeRect(boardPos.x * spaceWidth, boardPos.y * spaceHeight, spaceWidth, spaceHeight)
        ctx.strokeRect(boardPos2.x * spaceWidth, boardPos2.y * spaceHeight, spaceWidth, spaceHeight)
        ctx.closePath()
    }

    function updateGameStatus(message = "") {
        if (message == "") {
            let str = game.getTurn() == Colors.WHITE ? "WHITE TURN" : "BLACK TURN"
            if (game.isCheckmate()) {
                str += ": CHECKMATE!"
                let max = 8
                chessOof.src = `chessOof/${Math.ceil(Math.random() * (max - 1 + 1))}.gif`
                chessOof.style.display = "block"
            } else if (game.isStalemate()) {
                str += ": STALEMATE!"
            } else if (game.isCheck()) {
                str += ": CHECK!"
            } else if (game.getTurn() != playerColor) str += " (thinking)"
            gameStatusLabel.textContent = str
        } else {
            gameStatusLabel.textContent = message
        }
    }

    function updateMoveHistory() {
        moveHistory = game.getMoveHistory()
        while (gameHistoryArea.lastChild) {
            gameHistoryArea.removeChild(gameHistoryArea.lastChild)
        }
        moveHistory.forEach(move => {
            let li = document.createElement("li")
            li.textContent = move
            gameHistoryArea.appendChild(li)
        })
        gameHistoryArea.scrollTo(0, gameHistoryArea.scrollHeight)
    }


    function resetGame() {
        engine.setLevel(parseInt(engineLevelSelect.value))
        switch (playerColorSelect.value) {
            case "w":
                playerColor = Colors.WHITE
                break
            case "b":
                playerColor = Colors.BLACK
                break
            case "r":
                playerColor = Math.random() > 0.5 ? Colors.WHITE : Colors.BLACK
                break
        }
        lastMoved = null
        pieceSelected = null
        pieceValidMoves = null
        moveHistory = []
        chessOof.style.display = "none"
        game.reset()
        drawBoard()
        updateGameStatus()
        updateMoveHistory()
        setTimeout(makeEnemyTurn, 250)
    }

    function showValidMoves() {
        if (pieceValidMoves == null) return
        ctx.fillStyle = "green"
        pieceValidMoves.forEach(validMove => {
            ctx.beginPath()
            const boardPos = upsidedownAdj(validMove)
            ctx.arc(boardPos.x * spaceWidth + spaceWidth / 2, boardPos.y * spaceHeight + spaceHeight / 2, 10, 0, 2 * Math.PI)
            ctx.fill()
            ctx.closePath()
        })
        ctx.beginPath()
        ctx.lineWidth = 5
        ctx.strokeStyle = "blue"
        const boardPos = upsidedownAdj(pieceSelected)
        ctx.strokeRect(
            boardPos.x * spaceWidth,
            boardPos.y * spaceHeight,
            spaceWidth, spaceHeight
        )
        ctx.closePath()
    }

    function mouseToBoardPosition() {
        const boardPos = upsidedownAdj({
            x: Math.floor(mousePosition.x / spaceWidth),
            y: Math.floor(mousePosition.y / spaceHeight)
        })
        return boardPos
    }

    function drawBoard() {
        const upsidedown = playerColor == Colors.BLACK
        ctx.beginPath()
        const brown = "rgb(180, 136, 102)"
        const yellow = "rgb(239, 216, 183)"
        const letters = "abcdefgh"
        ctx.fillStyle = brown
        ctx.fillRect(0, 0, width, height)
        ctx.font = "16px Arial"
        for (let x = 0; x < 8; x++) {
            for (let y = 7; y != -1; y--) {
                ctx.fillStyle = yellow
                if ((x + y) % 2 == 0) ctx.fillRect(x * spaceWidth, y * spaceHeight, spaceWidth, spaceHeight)
                if (x == 0) {
                    ctx.fillStyle = "#4a3300"
                    ctx.fillText(y + 1, x + 2, y * spaceHeight + 18)
                }
                const boardPos = upsidedownAdj({x: x, y: y})
                const piece = game.getPiece(boardPos.x, boardPos.y)
                if (piece != null) {
                    let sprite = new Image()
                    sprite.src = `pieces/${piece.color == Colors.WHITE ? "white" : "black"}/${piece.type}.png`
                    sprite.onload = function() {
                        let scale = 0.18
                        let sw = sprite.width * scale
                        let sh = sprite.height * scale
                        ctx.drawImage(sprite,
                            x * spaceWidth + spaceWidth / 2 - sw / 2,
                            y * spaceHeight + spaceHeight / 2 - sh / 2,
                            sw, sh
                        )
                        afterPieceLoad()
                    }
                }
            }
            ctx.fillStyle = "#4a3300"
            ctx.fillText(letters[x], (x + 1) * width / 8 - 12, height - 5)
        }
        ctx.closePath()
        function afterPieceLoad() {
            showLastMoved()
            showValidMoves()
        }
    }

    function upsidedownAdj (pos) {
        const upsidedown = playerColor == Colors.BLACK
        return {
            x: upsidedown ? 7 - pos.x : pos.x,
            y: upsidedown ? pos.y : 7 - pos.y
        }
    }
}