# Automatic Chess Clock

Automatic Chess clock uses a Raspberry Pi and a camera to detect completed chess moves and automatically switch the active player's timer. It is a proof-of-concept focused on reliable timekeeping and robust move detection (board occupancy only) rather than move legality or piece recognition.

Key features
- Automatic move detection using per-square occupancy changes
- Deterministic, thread-separated architecture (Vision → FSM → Clock → UI)
- Accurate timekeeping using a monotonic clock source; supports increments and pauses
- Configurable stability timeout to tolerate piece adjustments and transient motion

Architecture (brief)
- Vision thread: captures frames and emits occupancy events
- FSM thread: confirms moves from occupancy events and emits MoveConfirmed
- Clock thread: maintains player clocks and switches on MoveConfirmed
- UI thread: displays time and accepts configuration (not authoritative)

Quick start
- Build (project root):

```bash
./build.sh
```

- Run the built executable from the `build` directory (or use your usual CMake run step).

Configuration
- Project configuration lives in [software/config/Config.json](software/config/Config.json).

Where to look
- Entry point: [software/src/main.cpp](software/src/main.cpp)
- Core modules: [software/include/core](software/include/core)
- Vision: [software/include/perception](software/include/perception)

License
- See [LICENSE](LICENSE)

