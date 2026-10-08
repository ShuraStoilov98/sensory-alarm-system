# Firmware behavior tests

Run `python3 test/run_host_tests.py` from the repository root with Python 3 and a C++17 compiler on Linux, macOS or WSL. Set `CXX` if the compiler is not available as `c++`, for example `CXX=g++ python3 test/run_host_tests.py`.

The host harness includes the actual `src/esp32/main.cpp` with fake Arduino GPIO, WiFi and Preferences implementations. Each scenario runs in its own process. Time conversion uses the host C library; GPIO step edges and elapsed time are simulated.

The checks exercise offline manual control, reconnect, boot with a held button, debounce and raw release, manual control after the alarm, full-minute scheduling, date persistence and rollback, latched faults, limit/direction behavior, manual override, failed storage, timer rollover and Bulgarian seasonal time.

They do not verify physical pin mapping, current limits, ESP32 background scheduling, electrical faults, motor acceleration, actual pulse timing or curtain mechanics. Record those separately in [Validation.md](../docs/Validation.md).

Temporary executables are created outside the repository and removed after each run. Test credentials are placeholders in `host/stubs/secrets.h`; the harness never reads local WiFi credentials.
