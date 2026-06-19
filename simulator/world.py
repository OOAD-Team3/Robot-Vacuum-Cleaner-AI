"""Grid world used by the Pygame simulator.

The world is intentionally Python-side only. It provides environment physics
and sensor calculation while the C++ controller still decides DRIVE commands.
"""

from __future__ import annotations

from collections import deque
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Set, Tuple


DEFAULT_MAP = [
    "##########",
    "#D......D#",
    "#D######.#",
    "#.#....#.#",
    "#.#.##.#.#",
    "#.#.##.#.#",
    "#.#.#DD#.#",
    "#.#.####.#",
    "#.#DD...D#",
    "#R########",
]


MAP_PRESETS: Dict[str, List[str]] = {
    "basic": DEFAULT_MAP,
    "open_room": [
        "############",
        "#..........#",
        "#.R........#",
        "#..........#",
        "#.....D....#",
        "#..D.......#",
        "############",
    ],
    "front_obstacle": [
        "########",
        "#..#...#",
        "#..R...#",
        "#...D..#",
        "#......#",
        "########",
    ],
    "three_side_back_clear": [
        "#####",
        "##R##",
        "#...#",
        "#.D.#",
        "#####",
    ],
    "deep_dead_end": [
        "##############",
        "#R....#......#",
        "#.###.#.####.#",
        "#...#...#D...#",
        "###.#####.####",
        "#...D....#...#",
        "#.######.#.#.#",
        "#........D.#.#",
        "##############",
    ],
    "busy_room": [
        "##############",
        "#R..D....#...#",
        "#.####.#.#.#.#",
        "#......#...#D#",
        "###.######.#.#",
        "#...D......#.#",
        "#.###.####.#.#",
        "#.....#..D...#",
        "##############",
    ],
    "obstacle_dense": [
        "##############",
        "#R...D...#...#",
        "#.##.###.#.#.#",
        "#....#.....#D#",
        "####.#.#####.#",
        "#D...#.......#",
        "#.#######.##.#",
        "#.......D....#",
        "##############",
    ],
}


DIRECTIONS = ("N", "E", "S", "W")
DELTAS = {
    "N": (0, -1),
    "E": (1, 0),
    "S": (0, 1),
    "W": (-1, 0),
}


def preset_names() -> List[str]:
    return list(MAP_PRESETS.keys())


def load_preset(name: str) -> List[str]:
    if name not in MAP_PRESETS:
        raise ValueError(f"unknown map preset {name!r}")

    return list(MAP_PRESETS[name])


@dataclass(frozen=True)
class Position:
    x: int
    y: int


@dataclass(frozen=True)
class StepResult:
    moved: bool
    blocked: bool
    message: str


def load_map_lines(path: Optional[str]) -> List[str]:
    if path is None:
        return load_preset("basic")

    return Path(path).read_text(encoding="utf-8").splitlines()


class GridWorld:
    def __init__(self, map_lines: Iterable[str], name: str = "custom") -> None:
        self.replace_map(map_lines, name)

    def replace_map(self, map_lines: Iterable[str], name: str = "custom") -> None:
        normalized = [line.rstrip("\n") for line in map_lines if line.strip()]
        if not normalized:
            raise ValueError("map is empty")

        width = len(normalized[0])
        if any(len(line) != width for line in normalized):
            raise ValueError("all map rows must have the same width")

        self._initial_lines = normalized
        self._width = width
        self._height = len(normalized)
        self._initial_walls: Set[Tuple[int, int]] = set()
        self._initial_dust: Set[Tuple[int, int]] = set()
        self._initial_robot = Position(1, 1)

        robot_count = 0
        for y, line in enumerate(normalized):
            for x, char in enumerate(line):
                if char == "#":
                    self._initial_walls.add((x, y))
                elif char == "D":
                    self._initial_dust.add((x, y))
                elif char == "R":
                    self._initial_robot = Position(x, y)
                    robot_count += 1
                elif char != ".":
                    raise ValueError(f"unsupported map character {char!r}")

        if robot_count != 1:
            raise ValueError("map must contain exactly one R")

        self._map_name = name
        self._reachable_floor = self._compute_reachable_floor()
        self.reset()

    @property
    def map_name(self) -> str:
        return self._map_name

    @property
    def width(self) -> int:
        return self._width

    @property
    def height(self) -> int:
        return self._height

    @property
    def robot(self) -> Position:
        return self._robot

    @property
    def direction(self) -> str:
        return self._direction

    @property
    def initial_robot(self) -> Position:
        return self._initial_robot

    @property
    def last_physics_message(self) -> str:
        return self._last_physics_message

    def reset(self) -> None:
        self._walls = set(self._initial_walls)
        self._dust = set(self._initial_dust)
        self._cleaned: Set[Tuple[int, int]] = set()
        self._robot = self._initial_robot
        self._direction = "N"
        self._last_physics_message = "world reset"
        self._mark_current_cell_clean()

    def reset_dust(self) -> None:
        self._dust = set(self._initial_dust)
        self._cleaned.clear()
        self._mark_current_cell_clean()
        self._last_physics_message = "dust reset"

    def is_wall(self, x: int, y: int) -> bool:
        return x < 0 or y < 0 or x >= self._width or y >= self._height or (x, y) in self._walls

    def has_dust(self, x: int, y: int) -> bool:
        return (x, y) in self._dust

    def is_cleaned(self, x: int, y: int) -> bool:
        return (x, y) in self._cleaned

    def cleaned_count(self) -> int:
        return len(self._cleaned.intersection(self._reachable_floor))

    def cleanable_count(self) -> int:
        return len(self._reachable_floor)

    def coverage_complete(self) -> bool:
        return self.cleaned_count() >= self.cleanable_count()

    def current_cell_has_dust(self) -> bool:
        return self.has_dust(self._robot.x, self._robot.y)

    def mark_current_dust_detected(self) -> None:
        self._mark_current_cell_clean()
        self._last_physics_message = "dust detected"

    def sensor_values(self, back_unknown: bool = False, coverage_bias: bool = True) -> Tuple[str, str, str, str]:
        coverage_open_neighbors = self._coverage_open_neighbors() if coverage_bias else None
        front = self._obstacle_in_relative_direction(0, coverage_open_neighbors)
        back = "UNKNOWN" if back_unknown else self._obstacle_in_relative_direction(2, coverage_open_neighbors)
        left = self._obstacle_in_relative_direction(-1, coverage_open_neighbors)
        right = self._obstacle_in_relative_direction(1, coverage_open_neighbors)
        return front, back, left, right

    def sensor_snapshot_command(self, back_unknown: bool = False, coverage_bias: bool = True) -> str:
        front, back, _left, _right = self.sensor_values(back_unknown, coverage_bias)
        dust = "1" if self.current_cell_has_dust() else "0"
        return f"SET_SENSOR_SNAPSHOT FRONT={front} BACK={back} DUST={dust}"

    def apply_drive(self, drive: str) -> StepResult:
        if drive == "MOVE_FORWARD":
            return self._move_by_direction(self._direction, "moved forward", "front blocked by world")
        if drive == "MOVE_BACKWARD":
            return self._move_by_direction(self._opposite_direction(), "moved backward", "back blocked by world")
        if drive in ("TURN_COUNTER_CLOCKWISE_90", "TURN_LEFT"):
            self._direction = self._relative_direction(-1)
            self._last_physics_message = "turned counter-clockwise"
            return StepResult(False, False, self._last_physics_message)
        if drive in ("TURN_CLOCKWISE_90", "TURN_RIGHT"):
            self._direction = self._relative_direction(1)
            self._last_physics_message = "turned clockwise"
            return StepResult(False, False, self._last_physics_message)
        if drive == "STOP":
            self._last_physics_message = "stopped"
            return StepResult(False, False, self._last_physics_message)

        self._last_physics_message = "no movement"
        return StepResult(False, False, self._last_physics_message)

    def _move_by_direction(self, direction: str, moved_message: str, blocked_message: str) -> StepResult:
        dx, dy = DELTAS[direction]
        next_x = self._robot.x + dx
        next_y = self._robot.y + dy
        if self.is_wall(next_x, next_y):
            self._last_physics_message = blocked_message
            return StepResult(False, True, blocked_message)

        self._robot = Position(next_x, next_y)
        self._mark_current_cell_clean()
        self._last_physics_message = moved_message
        return StepResult(True, False, moved_message)

    def _mark_current_cell_clean(self) -> None:
        self._cleaned.add((self._robot.x, self._robot.y))

    def _compute_reachable_floor(self) -> Set[Tuple[int, int]]:
        start = (self._initial_robot.x, self._initial_robot.y)
        reachable = {start}
        pending = [start]

        while pending:
            current_x, current_y = pending.pop()
            for dx, dy in DELTAS.values():
                x = current_x + dx
                y = current_y + dy
                if x < 0 or y < 0 or x >= self._width or y >= self._height:
                    continue
                if (x, y) in self._initial_walls or (x, y) in reachable:
                    continue

                reachable.add((x, y))
                pending.append((x, y))

        return reachable

    def _obstacle_in_relative_direction(self, offset: int, coverage_open_neighbors: Optional[Set[Tuple[int, int]]]) -> str:
        direction = self._relative_direction(offset)
        dx, dy = DELTAS[direction]
        target_x = self._robot.x + dx
        target_y = self._robot.y + dy

        if self.is_wall(target_x, target_y):
            return "1"

        if coverage_open_neighbors is not None and (target_x, target_y) not in coverage_open_neighbors:
            return "1"

        return "0"

    def _coverage_open_neighbors(self) -> Optional[Set[Tuple[int, int]]]:
        targets = self._coverage_targets()
        if not targets:
            return None

        candidates = []
        for dx, dy in DELTAS.values():
            x = self._robot.x + dx
            y = self._robot.y + dy
            if not self.is_wall(x, y):
                distance = self._distance_to_nearest_target((x, y), targets)
                if distance is not None:
                    candidates.append(((x, y), distance))

        if not candidates:
            return None

        best_distance = min(distance for _, distance in candidates)
        return {position for position, distance in candidates if distance == best_distance}

    def _coverage_targets(self) -> Set[Tuple[int, int]]:
        return self._reachable_floor.difference(self._cleaned)

    def _distance_to_nearest_target(self, start: Tuple[int, int], targets: Set[Tuple[int, int]]) -> Optional[int]:
        if start in targets:
            return 0

        visited = {start}
        pending = deque([(start, 0)])
        while pending:
            (current_x, current_y), distance = pending.popleft()
            for dx, dy in DELTAS.values():
                x = current_x + dx
                y = current_y + dy
                position = (x, y)
                if position in visited or self.is_wall(x, y):
                    continue
                if position in targets:
                    return distance + 1

                visited.add(position)
                pending.append((position, distance + 1))

        return None

    def _relative_direction(self, offset: int) -> str:
        current_index = DIRECTIONS.index(self._direction)
        return DIRECTIONS[(current_index + offset) % len(DIRECTIONS)]

    def _opposite_direction(self) -> str:
        return self._relative_direction(2)
