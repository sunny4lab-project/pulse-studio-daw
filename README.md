# Pulse Studio Enterprise 0.17.0
Experimental native C++17 / JUCE 8.0.11 application. Not production-ready.

## Mixer
New sessions begin with zero audio input channels and exactly ten renameable stereo summing buses, plus the stereo master. In Mixer, select **+ Audio channel** to add as many audio channels as available resources permit. **Delete channel** opens a channel selection menu. Stop live monitoring, playback and recording before structural edits. New channels are muted and have no assigned input. Open Device to enable physical inputs, then Details to select each mono input or stereo pair. Channels may share a physical input; the number of independent physical signals depends on the interface and driver. Each channel has native processing, two stereo insert slots and a selected bus. The ten bus racks also have two inserts each. The three auxiliary output pairs are separate from the ten summing buses.

There is no fixed software mixer channel limit. CPU, memory, interface capacity, plugin licenses and device settings constrain practical session size. Four-stem arrangement playback and 32-input raw multichannel WAV recording are separate existing subsystems with their own limits.

## Windows Dante / ASIO
1. Close any application using the same ASIO driver if it prevents access. Start the separately installed/licensed Dante Virtual Soundcard in ASIO mode (e.g. 64 x 64 at 48 kHz, subject to the device and network).
2. Open Dante Center and click Scan ASIO drivers to list registered drivers. Click Open audio devices, choose ASIO, then select Dante Virtual Soundcard. Configure the desired sample rate, buffer and active input/output channels in the device dialog.
3. Enable up to the available device inputs. The mixer input selectors show the actual driver-reported names of the active channels. Routing uses active-channel order; recheck assignments after changing the enabled channel set.
4. Select the stereo master output pair first; up to four active stereo pairs are routed (master + three auxiliary mixes). The engine does not expose 64 independent output mixes. Existing multichannel WAV capture is still limited to 32 inputs.
5. Use Save device setup in Dante Center to save the current driver, sample rate, buffer and channel masks locally. The setup is recalled when opening Device on the next launch, with live monitoring off. If a saved device is unavailable, JUCE may fall back to a default device; verify the active driver before enabling live.
6. Dante network subscriptions and clocking remain in Dante Controller. The application is an ASIO host, not a Dante network controller.

The Windows CI smoke test verifies ASIO backend registration without requiring physical audio hardware. This does not prove that DVS, WING or any particular driver works on a specific PC. The bundled ASIO SDK and JUCE have licensing requirements; review the applicable terms before distributing the binary.

## Projects
Version 3 .pulse projects store the variable channel count, routing, ten bus settings, effects, stem paths and plugin state. Older 32-channel projects can be migrated; routes to buses 11–32 are assigned to bus 10, and removed bus settings are not recalled. Projects containing plugins on removed buses are rejected until those plugins are moved or removed in an older version. Save a copy of the original project before migration. Original media and compatible plugins must remain available.

## Build
Install Git, CMake 3.22+, and a C++17 compiler (Visual Studio 2022 with Desktop C++ on Windows; Xcode on macOS). Linux dependencies are listed in .github/workflows/build.yml. Internet is required to fetch JUCE during configuration.

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --parallel 2
    ctest --test-dir build -C Release --output-on-failure

GitHub Actions builds Windows, macOS and Linux, with a Windows installer and native GUI screenshots. Windows builds enable ASIO using the ASIO headers bundled with JUCE 8.0.11. An external compatible SDK may optionally be supplied via -DASIO_SDK_DIR=/path/to/sdk. Dante DVS is separately installed/licensed. Review JUCE and ASIO licensing before distribution.

## Validation
Native regression tests include adding 128 mixer channels, deleting a middle channel, verifying stable channel/rack identities, routing audio on bus 10, project restoration, and returning to an empty session. Compilation and hardware verification must be checked for the specific commit. Test with headphones and low levels; never assume production readiness. Plugin latency compensation is not yet connected to the live graph; channel/bus insert effects must report zero latency.
