#!/bin/bash
# Double-click this file on a Mac to serve the app over http://localhost:8765
# (Use this only if opening index.html directly doesn't let you connect.)
# Keep this window open while using the app. Press Ctrl+C or close it to stop.

cd "$(dirname "$0")" || exit 1
PORT=8765
URL="http://localhost:$PORT"

echo "Serving RGB Strip Remote at $URL"
echo "Opening Chrome... (leave this window open)"
echo

# Try to open in Chrome specifically; fall back to the default browser.
open -a "Google Chrome" "$URL" 2>/dev/null || open "$URL"

# Python 3 ships with macOS / Homebrew.
exec python3 -m http.server "$PORT"
