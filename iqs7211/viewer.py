import sys
import re
import queue
import threading
import tkinter as tk
from collections import deque

# ============================================================
# Configuration
# ============================================================

MAX_SIZE = 500
MAX_VALUE = 0x3FFF
POINT_RADIUS = 5
POLL_INTERVAL_MS = 30
TRACE_LINES = 5

LINE_PATTERN = re.compile(r"@([0-9]+)'([0-9]+)#")

# Thread-safe communication between stdin reader and GUI
input_queue = queue.Queue()


# ============================================================
# Input handling
# ============================================================


def stdin_reader():
    """
    Read stdin in a background thread.

    Every received line is forwarded to the GUI, including
    invalid lines. EOF is reported explicitly.
    """
    try:
        while True:
            line = sys.stdin.readline()

            if line == "":
                input_queue.put(("EOF", None))
                break

            # Remove only the line terminator, not other whitespace
            line = line.rstrip("\r\n")
            input_queue.put(("LINE", line))

    except Exception as exc:
        input_queue.put(("ERROR", str(exc)))


def parse_line(line):
    """
    Return (x, y, status).

    Invalid input never raises an exception.
    """
    match = LINE_PATTERN.fullmatch(line)

    if not match:
        return None, None, "INVALID FORMAT"

    try:
        x = int(match.group(1))
        y = int(match.group(2))
    except ValueError:
        return None, None, "INVALID NUMBER"

    if not (0 <= x <= MAX_VALUE):
        return None, None, "X OUT OF RANGE"

    if not (0 <= y <= MAX_VALUE):
        return None, None, "Y OUT OF RANGE"

    return x, y, "OK"


# ============================================================
# GUI callbacks
# ============================================================

trace = deque(maxlen=TRACE_LINES)
last_point = None
stdin_finished = False


def draw_point(x, y):
    """Draw a point, with (0, 0) at the bottom-left."""
    px = x * (MAX_SIZE - 1) / MAX_VALUE
    py = (MAX_VALUE - y) * (MAX_SIZE - 1) / MAX_VALUE

    canvas.delete("point")

    canvas.create_oval(
        px - POINT_RADIUS,
        py - POINT_RADIUS,
        px + POINT_RADIUS,
        py + POINT_RADIUS,
        fill="red",
        outline="black",
        tags="point",
    )


def render_trace():
    """Refresh the trace area with the last five input lines."""
    trace_list.delete(0, tk.END)

    for raw_line, status in trace:
        # repr() makes blank lines and whitespace visible
        display_line = repr(raw_line)
        trace_list.insert(tk.END, f"{display_line}  ->  {status}")


def process_line(line):
    """Record every input line and update the point if valid."""
    global last_point

    x, y, status = parse_line(line)

    # Record valid and invalid lines alike
    trace.append((line, status))
    render_trace()

    if status != "OK":
        return

    last_point = (x, y)

    coordinates_label.config(text=f"X: {x}    Y: {y}")
    draw_point(x, y)


def poll_input():
    """Process queued input without blocking Tkinter."""
    global stdin_finished

    # Limit work per callback to keep the GUI responsive
    for _ in range(500):
        try:
            kind, data = input_queue.get_nowait()
        except queue.Empty:
            break

        if kind == "LINE":
            process_line(data)

        elif kind == "EOF":
            stdin_finished = True
            status_label.config(text="stdin closed — displaying last point")

        elif kind == "ERROR":
            stdin_finished = True
            status_label.config(text=f"stdin error: {data}")

    root.after(POLL_INTERVAL_MS, poll_input)


def clear_point():
    """Clear the point without clearing the input trace."""
    global last_point

    last_point = None
    canvas.delete("point")
    coordinates_label.config(text="X: --    Y: --")


# ============================================================
# Build the GUI
# ============================================================

root = tk.Tk()
root.title("Coordinate Viewer")
root.resizable(False, False)

coordinates_label = tk.Label(
    root,
    text="X: --    Y: --",
    font=("Arial", 14),
)
coordinates_label.pack(pady=(10, 5))

canvas = tk.Canvas(
    root,
    width=MAX_SIZE,
    height=MAX_SIZE,
    bg="white",
    highlightthickness=1,
    highlightbackground="black",
)
canvas.pack(padx=10, pady=5)

# Draw a subtle center crosshair
center = (MAX_SIZE - 1) / 2
canvas.create_line(center, 0, center, MAX_SIZE, fill="#dddddd")
canvas.create_line(0, center, MAX_SIZE, center, fill="#dddddd")

status_label = tk.Label(
    root,
    text="Waiting for stdin...",
    anchor="w",
    font=("Arial", 9),
)
status_label.pack(fill="x", padx=10, pady=5)

# Input trace at the bottom
trace_frame = tk.LabelFrame(
    root,
    text="Last 5 input lines (newest at bottom)",
    padx=5,
    pady=5,
)
trace_frame.pack(fill="x", padx=10, pady=(0, 10))

trace_list = tk.Listbox(
    trace_frame,
    width=65,
    height=TRACE_LINES,
    font=("Courier New", 9),
    activestyle="none",
)
trace_list.pack(fill="x")

# Start the reader. It never accesses Tkinter widgets.
reader_thread = threading.Thread(
    target=stdin_reader,
    name="stdin-reader",
    daemon=True,
)
reader_thread.start()

root.after(POLL_INTERVAL_MS, poll_input)
root.mainloop()
