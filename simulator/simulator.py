#!/usr/bin/env python3
"""Pygame simulator for the RVC TCP command protocol."""

from __future__ import annotations

import argparse
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Optional, Tuple

try:
    from .protocol_client import ProtocolError, RvcClient, parse_state_response
    from .world import DELTAS, GridWorld, load_map_lines, load_preset, preset_names
except ImportError:
    from protocol_client import ProtocolError, RvcClient, parse_state_response
    from world import DELTAS, GridWorld, load_map_lines, load_preset, preset_names


MAP_MODE = "map"
MANUAL_MODE = "manual"
PLAY_INTERVAL_SECONDS = 0.45
DEFAULT_DUST_POWER_TIMEOUT_SECONDS = 5.0


@dataclass
class SimulatorModel:
    state: Dict[str, str] = field(default_factory=dict)
    last_command: str = ""
    last_response: str = ""
    error_message: str = ""
    connected: bool = False
    mode: str = MAP_MODE
    playing: bool = False
    physics_message: str = ""
    coverage_bias: bool = True
    map_names: list[str] = field(default_factory=list)
    map_index: int = 0
    power_timeout_deadline: Optional[float] = None
    timer_active_observed: bool = False
    dust_timeout_seconds: float = DEFAULT_DUST_POWER_TIMEOUT_SECONDS


def manual_command_for_key(key: int, pygame_module) -> Optional[Tuple[str, bool]]:
    mapping = {
        pygame_module.K_1: ("SET_FRONT 0", True),
        pygame_module.K_2: ("SET_FRONT 1", True),
        pygame_module.K_3: ("SET_BACK 0", True),
        pygame_module.K_4: ("SET_BACK 1", True),
        pygame_module.K_5: ("SET_BACK UNKNOWN", True),
        pygame_module.K_q: ("SET_SIDE LEFT=1 RIGHT=0", True),
        pygame_module.K_w: ("SET_SIDE LEFT=0 RIGHT=1", True),
        pygame_module.K_e: ("SET_SIDE LEFT=0 RIGHT=0", True),
        pygame_module.K_a: ("SET_OBSTACLES FRONT=0 BACK=UNKNOWN LEFT=0 RIGHT=0", True),
        pygame_module.K_s: ("SET_OBSTACLES FRONT=1 BACK=UNKNOWN LEFT=0 RIGHT=0", True),
        pygame_module.K_d: ("SET_OBSTACLES FRONT=1 BACK=0 LEFT=1 RIGHT=1", True),
        pygame_module.K_f: ("SET_OBSTACLES FRONT=1 BACK=1 LEFT=1 RIGHT=1", True),
        pygame_module.K_z: ("DUST_DETECTED", True),
        pygame_module.K_x: ("POWER_TIMEOUT", True),
    }
    return mapping.get(key)


def clear_power_timer_tracking(model: SimulatorModel) -> None:
    model.power_timeout_deadline = None
    model.timer_active_observed = False


def force_power_timer_resync(model: SimulatorModel) -> None:
    clear_power_timer_tracking(model)


def sync_power_timer_tracking(model: SimulatorModel) -> None:
    timer_active = model.state.get("TIMER_ACTIVE") == "1"
    if timer_active:
        if not model.timer_active_observed or model.power_timeout_deadline is None:
            model.power_timeout_deadline = time.monotonic() + model.dust_timeout_seconds
        model.timer_active_observed = True
        return

    clear_power_timer_tracking(model)


def update_state_from_response(model: SimulatorModel, response: str) -> None:
    if response.startswith("OK STATE"):
        model.state = parse_state_response(response)
        sync_power_timer_tracking(model)


def ensure_connected(client: RvcClient, model: SimulatorModel) -> bool:
    if client.connected:
        model.connected = True
        return True

    try:
        client.connect()
        model.connected = True
        return True
    except (ConnectionError, TimeoutError, OSError) as error:
        model.connected = False
        model.error_message = str(error)
        return False


def issue_command(
    client: RvcClient,
    model: SimulatorModel,
    command: str,
    refresh_state: bool = False,
) -> Optional[str]:
    if not ensure_connected(client, model):
        return None

    model.last_command = command
    model.error_message = ""

    try:
        response = client.send_command(command)
        model.last_response = response
        model.connected = client.connected
        update_state_from_response(model, response)

        if response.startswith("ERR "):
            model.error_message = response
            return response

        if command == "DUST_DETECTED":
            force_power_timer_resync(model)

        if refresh_state and command != "GET_STATE":
            refresh_state_after_command(client, model)

        return response
    except (ConnectionError, TimeoutError, ProtocolError, OSError) as error:
        model.connected = False
        model.error_message = str(error)
        return None


def request_state(client: RvcClient, model: SimulatorModel) -> Optional[Dict[str, str]]:
    response = issue_command(client, model, "GET_STATE", refresh_state=False)
    if response is None:
        return None

    try:
        model.state = parse_state_response(response)
        sync_power_timer_tracking(model)
        return model.state
    except ProtocolError as error:
        model.error_message = str(error)
        return None


def refresh_state_after_command(client: RvcClient, model: SimulatorModel) -> Optional[Dict[str, str]]:
    try:
        response = client.send_command("GET_STATE")
        model.last_response = response
        model.connected = client.connected
        model.state = parse_state_response(response)
        sync_power_timer_tracking(model)
        return model.state
    except (ConnectionError, TimeoutError, ProtocolError, OSError) as error:
        model.connected = False
        model.error_message = str(error)
        return None


def process_power_timer(client: RvcClient, model: SimulatorModel) -> None:
    if model.power_timeout_deadline is None:
        return

    if time.monotonic() < model.power_timeout_deadline:
        return

    response = issue_command(client, model, "POWER_TIMEOUT", refresh_state=True)
    if response is None or response.startswith("ERR "):
        clear_power_timer_tracking(model)


def reset_simulation(client: RvcClient, model: SimulatorModel, world: GridWorld) -> None:
    world.reset()
    model.physics_message = world.last_physics_message
    model.playing = False
    clear_power_timer_tracking(model)
    issue_command(client, model, "RESET", refresh_state=True)


def switch_map_preset(client: RvcClient, model: SimulatorModel, world: GridWorld, direction: int) -> None:
    if not model.map_names:
        model.error_message = "no map presets available"
        return

    model.map_index = (model.map_index + direction) % len(model.map_names)
    map_name = model.map_names[model.map_index]
    world.replace_map(load_preset(map_name), map_name)
    model.playing = False
    model.error_message = ""
    model.physics_message = f"map switched: {map_name}"
    clear_power_timer_tracking(model)
    issue_command(client, model, "RESET", refresh_state=True)
    model.physics_message = f"map switched: {map_name}"


def run_map_step(client: RvcClient, model: SimulatorModel, world: GridWorld) -> None:
    if not ensure_connected(client, model):
        return

    if world.current_cell_has_dust():
        response = issue_command(client, model, "DUST_DETECTED", refresh_state=False)
        if response is None or response.startswith("ERR "):
            return

        state = refresh_state_after_command(client, model)
        if state and (state.get("CLEANING_POWER") == "INCREASED" or state.get("DUST") == "1"):
            world.clear_current_dust()
            model.physics_message = world.last_physics_message
            stop_if_coverage_complete(model, world)
        return

    if stop_if_coverage_complete(model, world):
        return

    command = world.set_obstacles_command(back_unknown=False, coverage_bias=model.coverage_bias)
    response = issue_command(client, model, command, refresh_state=False)
    if response is None or response.startswith("ERR "):
        return

    state = refresh_state_after_command(client, model)
    if not state:
        return

    result = world.apply_drive(state.get("DRIVE", "NONE"))
    model.physics_message = result.message
    stop_if_coverage_complete(model, world)


def stop_if_coverage_complete(model: SimulatorModel, world: GridWorld) -> bool:
    if not world.coverage_complete():
        return False

    model.playing = False
    model.physics_message = "coverage complete"
    return True


def color_for_binary(value: str) -> Tuple[int, int, int]:
    return (218, 82, 82) if value == "1" else (74, 171, 112)


def draw_text(surface, font, text: str, x: int, y: int, color=(230, 234, 241)) -> None:
    rendered = font.render(text, True, color)
    surface.blit(rendered, (x, y))


def wrap_text(font, text: str, max_width: int) -> list[str]:
    if not text:
        return [""]

    lines: list[str] = []
    current = ""
    for word in text.split():
        candidate = word if not current else f"{current} {word}"
        if font.size(candidate)[0] <= max_width:
            current = candidate
            continue

        if current:
            lines.append(current)
            current = ""

        if font.size(word)[0] <= max_width:
            current = word
            continue

        fragment = ""
        for char in word:
            candidate_fragment = fragment + char
            if font.size(candidate_fragment)[0] <= max_width:
                fragment = candidate_fragment
            else:
                if fragment:
                    lines.append(fragment)
                fragment = char
        current = fragment

    if current:
        lines.append(current)

    return lines or [""]


def draw_wrapped_text(
    surface,
    font,
    text: str,
    x: int,
    y: int,
    max_width: int,
    color=(230, 234, 241),
    max_lines: int = 3,
) -> int:
    lines = wrap_text(font, text, max_width)
    if len(lines) > max_lines:
        lines = lines[:max_lines]
        ellipsis = "..."
        while lines[-1] and font.size(lines[-1] + ellipsis)[0] > max_width:
            lines[-1] = lines[-1][:-1]
        lines[-1] = lines[-1] + ellipsis

    line_height = font.get_linesize()
    for line in lines:
        draw_text(surface, font, line, x, y, color)
        y += line_height

    return y


def draw_world(surface, font, small_font, world: GridWorld, state: Dict[str, str]) -> None:
    import pygame

    map_rect = pygame.Rect(42, 104, 640, 500)
    tile_size = min(map_rect.width // world.width, map_rect.height // world.height)
    map_width = tile_size * world.width
    map_height = tile_size * world.height
    origin_x = map_rect.x + (map_rect.width - map_width) // 2
    origin_y = map_rect.y + (map_rect.height - map_height) // 2

    pygame.draw.rect(surface, (25, 31, 43), map_rect, border_radius=8)
    pygame.draw.rect(surface, (86, 101, 126), map_rect, 1, border_radius=8)

    for y in range(world.height):
        for x in range(world.width):
            tile = pygame.Rect(origin_x + x * tile_size, origin_y + y * tile_size, tile_size, tile_size)
            if world.is_wall(x, y):
                color = (69, 79, 99)
            elif world.has_dust(x, y):
                color = (58, 47, 43)
            elif world.is_cleaned(x, y):
                color = (36, 67, 58)
            else:
                color = (32, 39, 53)

            pygame.draw.rect(surface, color, tile)
            pygame.draw.rect(surface, (23, 28, 39), tile, 1)

            if world.has_dust(x, y):
                pygame.draw.circle(surface, (219, 185, 93), tile.center, max(4, tile_size // 7))
            elif world.is_cleaned(x, y) and not world.is_wall(x, y):
                pygame.draw.circle(surface, (83, 170, 125), tile.center, max(2, tile_size // 14))

    robot = world.robot
    robot_rect = pygame.Rect(
        origin_x + robot.x * tile_size,
        origin_y + robot.y * tile_size,
        tile_size,
        tile_size,
    )
    pygame.draw.circle(surface, (102, 179, 237), robot_rect.center, max(10, tile_size // 3))
    pygame.draw.circle(surface, (216, 232, 247), robot_rect.center, max(10, tile_size // 3), 2)

    dx, dy = DELTAS[world.direction]
    arrow_end = (
        robot_rect.centerx + int(dx * tile_size * 0.32),
        robot_rect.centery + int(dy * tile_size * 0.32),
    )
    pygame.draw.line(surface, (18, 24, 35), robot_rect.center, arrow_end, 4)
    pygame.draw.circle(surface, (18, 24, 35), arrow_end, 4)

    draw_text(surface, small_font, f"Robot: ({robot.x}, {robot.y}) {world.direction}", 48, 616, (200, 210, 226))
    draw_text(surface, small_font, f"World: {world.last_physics_message}", 48, 640, (200, 210, 226))

    sensors = [
        ("FRONT", state.get("FRONT", "-")),
        ("BACK", state.get("BACK", "-")),
        ("LEFT", state.get("LEFT", "-")),
        ("RIGHT", state.get("RIGHT", "-")),
    ]
    x = 48
    for label, value in sensors:
        text = f"{label} {value}"
        block_width = max(84, small_font.size(text)[0] + 18)
        color = (126, 142, 168) if value == "UNKNOWN" else color_for_binary(value)
        pygame.draw.rect(surface, color, pygame.Rect(x, 672, block_width, 28), border_radius=5)
        draw_text(surface, small_font, text, x + 8, 678, (18, 24, 35))
        x += block_width + 12


def draw_manual_robot(surface, font, state: Dict[str, str]) -> None:
    import pygame

    center = (360, 330)
    robot_rect = pygame.Rect(290, 260, 140, 140)
    pygame.draw.rect(surface, (43, 51, 68), robot_rect, border_radius=8)
    pygame.draw.rect(surface, (174, 188, 212), robot_rect, 3, border_radius=8)
    draw_text(surface, font, "RVC", center[0] - 22, center[1] - 12)

    sensors = {
        "FRONT": (center[0], robot_rect.top - 45),
        "BACK": (center[0], robot_rect.bottom + 45),
        "LEFT": (robot_rect.left - 55, center[1]),
        "RIGHT": (robot_rect.right + 55, center[1]),
    }

    for name, position in sensors.items():
        value = state.get(name, "UNKNOWN")
        color = (126, 142, 168) if value == "UNKNOWN" else color_for_binary(value)
        draw_text(surface, font, name, position[0] - 28, position[1] - 42, (200, 210, 226))

        if value == "UNKNOWN":
            marker_width = max(82, font.size(value)[0] + 18)
            marker = pygame.Rect(0, 0, marker_width, 36)
            marker.center = position
            pygame.draw.rect(surface, color, marker, border_radius=18)
            text_width, text_height = font.size(value)
            draw_text(
                surface,
                font,
                value,
                marker.centerx - text_width // 2,
                marker.centery - text_height // 2,
                (18, 24, 35),
            )
            continue

        pygame.draw.circle(surface, color, position, 20)
        text_width, text_height = font.size(value)
        draw_text(
            surface,
            font,
            value,
            position[0] - text_width // 2,
            position[1] - text_height // 2,
            (18, 24, 35),
        )


def draw_panel(surface, font, small_font, model: SimulatorModel, world: GridWorld) -> None:
    import pygame

    panel = pygame.Rect(720, 40, 520, 760)
    pygame.draw.rect(surface, (31, 38, 52), panel, border_radius=8)
    pygame.draw.rect(surface, (86, 101, 126), panel, 1, border_radius=8)

    content_x = panel.x + 22
    content_width = panel.width - 44
    y = panel.y + 22
    connection = "CONNECTED" if model.connected else "DISCONNECTED"
    connection_color = (94, 203, 128) if model.connected else (231, 91, 91)
    draw_text(surface, font, f"Connection: {connection}", content_x, y, connection_color)

    y += 34
    draw_text(surface, font, f"Mode: {model.mode.upper()}", content_x, y, (230, 234, 241))
    y += 28
    play_text = "PLAY" if model.playing else "PAUSE"
    draw_text(surface, font, f"Run: {play_text}", content_x, y, (230, 234, 241))

    y += 28
    bias_text = "ON" if model.coverage_bias else "OFF"
    draw_text(surface, font, f"Coverage assist: {bias_text}", content_x, y, (230, 234, 241))

    y += 28
    if model.map_names and world.map_name in model.map_names:
        map_text = f"{world.map_name} ({model.map_index + 1}/{len(model.map_names)})"
    else:
        map_text = world.map_name
    draw_text(surface, font, f"Map: {map_text}", content_x, y, (230, 234, 241))

    robot = world.robot
    y += 28
    draw_text(surface, font, f"Robot: x={robot.x} y={robot.y} dir={world.direction}", content_x, y)

    y += 28
    draw_text(surface, font, f"Cleaned: {world.cleaned_count()}/{world.cleanable_count()}", content_x, y)

    y += 36
    state_rows = [
        (("MOVEMENT", model.state.get("MOVEMENT", "-")), ("FRONT", model.state.get("FRONT", "-"))),
        (("DRIVE", model.state.get("DRIVE", "-")), ("BACK", model.state.get("BACK", "-"))),
        (("CLEANING_POWER", model.state.get("CLEANING_POWER", "-")), ("LEFT", model.state.get("LEFT", "-"))),
        (("TIMER_ACTIVE", model.state.get("TIMER_ACTIVE", "-")), ("RIGHT", model.state.get("RIGHT", "-"))),
        (("DUST", model.state.get("DUST", "-")), None),
    ]

    state_col2_x = content_x + 320
    for left, right in state_rows:
        key, value = left
        draw_text(surface, font, f"{key}: {value}", content_x, y)
        if right:
            key, value = right
            draw_text(surface, font, f"{key}: {value}", state_col2_x, y)
        y += 25

    y += 12
    draw_text(surface, font, "Last command", content_x, y, (200, 210, 226))
    y += 25
    y = draw_wrapped_text(surface, small_font, model.last_command or "-", content_x, y, content_width, (230, 234, 241), max_lines=2)

    y += 18
    draw_text(surface, font, "Last response", content_x, y, (200, 210, 226))
    y += 25
    response = model.last_response or "-"
    y = draw_wrapped_text(surface, small_font, response, content_x, y, content_width, (230, 234, 241), max_lines=4)

    y += 18
    draw_text(surface, font, "Physics", content_x, y, (200, 210, 226))
    y += 25
    y = draw_wrapped_text(surface, small_font, model.physics_message or "-", content_x, y, content_width, (230, 234, 241), max_lines=2)

    y += 18
    draw_text(surface, font, "Error", content_x, y, (200, 210, 226))
    y += 25
    error = model.error_message or "-"
    draw_wrapped_text(surface, small_font, error, content_x, y, content_width, (245, 142, 124), max_lines=2)


def draw_command_blocks(
    surface,
    small_font,
    blocks: list[Tuple[str, str]],
    start_x: int,
    start_y: int,
    max_width: int,
    row_height: int = 25,
    gap: int = 6,
) -> None:
    import pygame

    x = start_x
    y = start_y
    for command, description in blocks:
        prefix = f"{command} : "
        width = small_font.size(prefix + description)[0] + 14
        if x > start_x and x + width > start_x + max_width:
            x = start_x
            y += row_height + gap

        block = pygame.Rect(x, y, width, row_height)
        pygame.draw.rect(surface, (31, 38, 52), block, border_radius=6)
        pygame.draw.rect(surface, (86, 101, 126), block, 1, border_radius=6)

        text_x = x + 7
        text_y = y + max(2, (row_height - small_font.get_height()) // 2)
        draw_text(surface, small_font, prefix, text_x, text_y, (142, 201, 245))
        draw_text(surface, small_font, description, text_x + small_font.size(prefix)[0], text_y, (214, 221, 233))
        x += width + gap


def draw_help(surface, small_font, mode: str) -> None:
    if mode == MAP_MODE:
        help_font = small_font
        blocks = [
            ("Space", "step"),
            ("Enter", "play/pause"),
            ("N/V", "switch map"),
            ("R", "reset"),
            ("G", "GET_STATE"),
            ("P", "PING"),
            ("C", "reset dust"),
            ("B", "coverage assist"),
            ("M", "manual mode"),
            ("Esc", "exit"),
        ]
        draw_command_blocks(surface, help_font, blocks, 44, 752, 660)
    else:
        import pygame

        help_font = pygame.font.SysFont("Arial", 14)
        blocks = [
            ("G", "state"),
            ("P", "ping"),
            ("R", "reset"),
            ("M", "map"),
            ("1", "FRONT=0"),
            ("2", "FRONT=1"),
            ("3", "BACK=0"),
            ("4", "BACK=1"),
            ("5", "BACK=UNK"),
            ("Q", "L=1/R=0"),
            ("W", "L=0/R=1"),
            ("E", "L=0/R=0"),
            ("A", "clear"),
            ("S", "front"),
            ("D", "3-side"),
            ("F", "all"),
            ("Z", "dust"),
            ("X", "timeout"),
            ("Esc", "exit"),
        ]
        draw_command_blocks(surface, help_font, blocks, 44, 748, 660, row_height=20, gap=4)


def handle_key(
    key: int,
    pygame_module,
    client: RvcClient,
    model: SimulatorModel,
    world: GridWorld,
) -> None:
    if key == pygame_module.K_ESCAPE:
        raise KeyboardInterrupt

    if key == pygame_module.K_m:
        model.mode = MANUAL_MODE if model.mode == MAP_MODE else MAP_MODE
        model.playing = False
        model.error_message = ""
        return

    if key == pygame_module.K_p:
        issue_command(client, model, "PING", refresh_state=False)
        return

    if key == pygame_module.K_g:
        request_state(client, model)
        return

    if key == pygame_module.K_r:
        reset_simulation(client, model, world)
        return

    if key == pygame_module.K_n and model.mode == MAP_MODE:
        switch_map_preset(client, model, world, 1)
        return

    if key == pygame_module.K_v and model.mode == MAP_MODE:
        switch_map_preset(client, model, world, -1)
        return

    if key == pygame_module.K_c and model.mode == MAP_MODE:
        world.reset_dust()
        model.physics_message = world.last_physics_message
        return

    if key == pygame_module.K_b and model.mode == MAP_MODE:
        model.coverage_bias = not model.coverage_bias
        state = "on" if model.coverage_bias else "off"
        model.physics_message = f"coverage assist {state}"
        return

    if model.mode == MAP_MODE:
        if key == pygame_module.K_SPACE:
            run_map_step(client, model, world)
        elif key == pygame_module.K_RETURN:
            model.playing = not model.playing
        return

    mapped = manual_command_for_key(key, pygame_module)
    if mapped:
        command, refresh_state = mapped
        issue_command(client, model, command, refresh_state)


def run_simulator(
    host: str,
    port: int,
    mode: str,
    map_path: Optional[str],
    dust_timeout_seconds: float,
) -> int:
    import pygame

    available_maps = preset_names()
    if map_path:
        world = GridWorld(load_map_lines(map_path), Path(map_path).stem or "custom")
        initial_map_index = -1
    else:
        initial_map_name = available_maps[0]
        world = GridWorld(load_preset(initial_map_name), initial_map_name)
        initial_map_index = available_maps.index(initial_map_name)

    model = SimulatorModel(
        mode=mode,
        map_names=available_maps,
        map_index=initial_map_index,
        dust_timeout_seconds=dust_timeout_seconds,
    )
    client = RvcClient(host, port)

    pygame.init()
    pygame.display.set_caption("Robot Vacuum Cleaner Simulator")
    screen = pygame.display.set_mode((1280, 820))
    clock = pygame.time.Clock()
    font = pygame.font.SysFont("Arial", 20)
    small_font = pygame.font.SysFont("Arial", 16)
    title_font = pygame.font.SysFont("Arial", 28, bold=True)

    ensure_connected(client, model)
    request_state(client, model)

    running = True
    last_step_time = time.monotonic()
    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.KEYDOWN:
                try:
                    handle_key(event.key, pygame, client, model, world)
                except KeyboardInterrupt:
                    running = False

        now = time.monotonic()
        process_power_timer(client, model)
        if model.mode == MAP_MODE and model.playing and now - last_step_time >= PLAY_INTERVAL_SECONDS:
            run_map_step(client, model, world)
            last_step_time = now

        screen.fill((18, 24, 35))
        draw_text(screen, title_font, "Robot Vacuum Cleaner Simulator", 44, 34)
        draw_text(screen, small_font, f"TCP {host}:{port}", 46, 72, (184, 194, 212))
        if model.mode == MAP_MODE:
            draw_world(screen, font, small_font, world, model.state)
        else:
            draw_manual_robot(screen, font, model.state)
            draw_text(screen, small_font, "Manual sensor input mode", 48, 616, (200, 210, 226))

        power = model.state.get("CLEANING_POWER", "OFF")
        power_color = (96, 190, 240) if power == "INCREASED" else (94, 203, 128) if power == "NORMAL" else (126, 142, 168)
        draw_text(screen, title_font, f"CLEANING POWER: {power}", 44, 710, power_color)
        draw_panel(screen, font, small_font, model, world)
        draw_help(screen, small_font, model.mode)
        pygame.display.flip()
        clock.tick(30)

    client.close()
    pygame.quit()
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Pygame simulator for rvc_app TCP protocol")
    parser.add_argument("--host", default="127.0.0.1")
    parser.add_argument("--port", default=9090, type=int)
    parser.add_argument("--mode", choices=(MAP_MODE, MANUAL_MODE), default=MAP_MODE)
    parser.add_argument("--map", dest="map_path", default=None, help="Optional text map file")
    parser.add_argument(
        "--dust-timeout",
        default=DEFAULT_DUST_POWER_TIMEOUT_SECONDS,
        type=float,
        help="Seconds to wait before sending POWER_TIMEOUT after TIMER_ACTIVE=1",
    )
    args = parser.parse_args()
    if args.dust_timeout <= 0:
        parser.error("--dust-timeout must be greater than 0")
    return run_simulator(args.host, args.port, args.mode, args.map_path, args.dust_timeout)


if __name__ == "__main__":
    raise SystemExit(main())
