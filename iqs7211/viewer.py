#!/usr/bin/env python3

import sys
import tkinter as tk
import threading
import queue


MAX_VALUE = 0x3FFF

# Queue used to safely pass stdin data to the Tkinter thread.
data_queue = queue.Queue()


class PointViewer:
    def __init__(self, root):
        self.root = root
        self.root.title("XY Point Viewer")

        self.canvas_size = 500
        self.margin = 40

        # ---- Canvas ----
        self.canvas = tk.Canvas(
            root,
            width=self.canvas_size,
            height=self.canvas_size,
            bg="white",
            highlightthickness=1,
            highlightbackground="black",
        )
        self.canvas.pack(padx=10, pady=10)

        # Draw coordinate square.
        self.canvas.create_rectangle(
            self.margin,
            self.margin,
            self.canvas_size - self.margin,
            self.canvas_size - self.margin,
            outline="black",
        )

        # Axis labels
        self.canvas.create_text(
            self.canvas_size / 2,
            self.canvas_size - 15,
            text="X",
        )

        self.canvas.create_text(
            15,
            self.canvas_size / 2,
            text="Y",
            angle=90,
        )

        # ---- Information ----
        self.coordinate_label = tk.Label(
            root,
            text="X: --    Y: --",
            font=("TkDefaultFont", 14),
        )
        self.coordinate_label.pack(pady=(0, 5))

        self.status_label = tk.Label(
            root,
            text="Waiting for data...",
            font=("TkDefaultFont", 14),
            fg="green",
        )
        self.status_label.pack(pady=(0, 10))

        self.point = None

        # Check the input queue periodically.
        self.root.after(20, self.process_queue)

    def process_queue(self):
        try:
            while True:
                line = data_queue.get_nowait()
                self.process_line(line)
        except queue.Empty:
            pass

        self.root.after(20, self.process_queue)

    def process_line(self, line):
        line = line.strip()

        if not line:
            return

        # Split on arbitrary whitespace.
        fields = line.split()

        # We need at least:
        # field 0 = unused
        # field 1 = X
        # field 2 = Y
        if len(fields) < 3:
            self.show_error(f"Invalid input: {line}")
            return

        try:
            x = int(fields[1], 0)
            y = int(fields[2], 0)
        except ValueError:
            self.show_error(f"Invalid numbers: {line}")
            return

        self.coordinate_label.config(text=f"X: {x} (0x{x:X})    Y: {y} (0x{y:X})")

        # Check for error.
        if x > MAX_VALUE or y > MAX_VALUE:
            self.show_error(f"ERROR: coordinate exceeds 0x{MAX_VALUE:X}")
            return

        # Valid point.
        self.status_label.config(
            text="OK",
            fg="green",
        )

        self.draw_point(x, y)

    def show_error(self, message):
        self.status_label.config(
            text=message,
            fg="red",
        )

    def draw_point(self, x, y):
        # Remove previous point.
        if self.point is not None:
            self.canvas.delete(self.point)

        # Map 0..0x1000 to the square.
        plot_size = self.canvas_size - 2 * self.margin

        px = self.margin + (x / MAX_VALUE) * plot_size

        # Canvas Y coordinates increase downward,
        # so invert Y.
        py = self.canvas_size - self.margin - (y / MAX_VALUE) * plot_size

        radius = 6

        self.point = self.canvas.create_oval(
            px - radius,
            py - radius,
            px + radius,
            py + radius,
            fill="blue",
            outline="black",
        )


def stdin_reader():
    """
    Read stdin continuously.

    This runs in a separate thread so that blocking on stdin
    does not freeze the Tkinter GUI.
    """
    for line in sys.stdin:
        data_queue.put(line)

    # stdin closed
    data_queue.put("__EOF__")


def main():
    root = tk.Tk()

    viewer = PointViewer(root)

    # stdin must be read from a separate thread because
    # sys.stdin.readline() can block.
    thread = threading.Thread(
        target=stdin_reader,
        daemon=True,
    )
    thread.start()

    root.mainloop()


if __name__ == "__main__":
    main()
