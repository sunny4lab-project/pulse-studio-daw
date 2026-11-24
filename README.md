# Pulse Studio 0.2 Alpha
Experimental native JUCE 8.0.6 application. Not production ready.

## Build
Install Git, CMake 3.22+, and a C++17 compiler (Visual Studio 2022 with Desktop C++ on Windows; Xcode on macOS). Linux dependencies are listed in .github/workflows/build.yml. Internet is required to fetch JUCE during configuration.

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --parallel 2
    ctest --test-dir build -C Release --output-on-failure

Alternatively upload the contents of this directory, including hidden .github, to a new GitHub repository. In Actions select Pulse Studio build, run workflow, then download the artifact for your OS if successful. Artifacts are unsigned application builds, not installers. Windows ASIO requires a compatible SDK and -DASIO_SDK_DIR=/path/to/sdk; default CI does not enable ASIO. Dante DVS is separately installed/licensed. Review JUCE and ASIO licensing before distribution.

## Current source functionality
32 mono inputs mixed to stereo, gain/pan/mute/solo, basic high-pass and fixed -18 dBFS threshold compressor with adjustable ratio, smoothed channel gain, stereo file playback and WAV recording, device CPU and writer failure counter. Live inputs start disabled and muted. Use headphones and low levels when testing. Hard clipping protection is not a mastering limiter. Recording is stereo only; device changes during recording are unsupported. Write failure counter does not measure all possible dropouts.

## Validation
Standalone DSP tests passed using g++ in development environment. Full JUCE build not verified; dependency fetch failed in this environment. CI workflow supplied but not executed. No hardware or latency benchmarks performed.

## Roadmap / release gates
1. Compile on Windows/macOS/Linux; fix compiler issues; test audio devices, sample rates, recording integrity, shutdown and hotplug.
2. Replace callback locking and shared playback/record disk thread with bounded queues and separate workers; add accurate xrun and recording gap tracking; measure callback deadlines, RAM and CPU.
3. Add project persistence, stereo input pairs, buses, sends, multitrack recording and playback, VST3/AU scanning and hosting, delay compensation.
4. Add arrangement, MIDI, automation, editing, clip launching, setlists and keyboard layers.
5. Add native effects, background analysis, offline AI, remote control and broadcast tools.
6. Run 8-hour stress tests with WING USB/DVS, 48 kHz and multiple buffer sizes; compare identical workloads against Ableton and REAPER; create signed installers after verification.

No claim of superiority or live safety until tests pass. GUI concepts from prior conversation are design references, not implemented screens.
