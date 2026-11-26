# Pulse Studio Enterprise 0.19.1
Experimental native C++17 / JUCE 8.0.11 application. Not production-ready.

## Mixer
New sessions begin with zero audio input channels and exactly ten renameable stereo summing buses, plus the stereo master. In Mixer, select **+ Audio channel** to add as many audio channels as available resources permit. **Delete channel** opens a channel selection menu. Stop live monitoring, playback and recording before structural edits. New channels are muted and have no assigned input. Open Device to enable physical inputs, then Details to select each mono input or stereo pair. Channels may share a physical input; the number of independent physical signals depends on the interface and driver. Each channel has native processing, two stereo insert slots and a selected bus. The ten bus racks also have two inserts each. The three auxiliary output pairs are separate from the ten summing buses.

There is no fixed software mixer channel limit. CPU, memory, interface capacity, plugin licenses and device settings constrain practical session size. Four-stem arrangement playback and 32-input raw multichannel WAV recording are separate existing subsystems with their own limits.

## Universal Audio Device Manager
1. Connect a compatible USB, Thunderbolt, built-in or network audio interface and install its required operating system driver. Close any other application that exclusively occupies the driver.
2. Open Audio Device Manager and click **Scan all devices** to list registered backends and input/output endpoints. Scanning does not select or open an endpoint for audio streaming. Scan only with live monitoring, playback and recording stopped.
3. Click **Configure device** and select the backend and device. On Windows, ASIO is preferred for professional multichannel audio. Other backends are available as provided by JUCE and the platform. On macOS choose CoreAudio; on Linux choose an available backend.
4. Select sample rate, buffer size and active input/output channels. The mixer displays driver-reported names in active-channel order. Recheck assignments if the active channel mask or device changes.
5. The engine supports up to four active stereo output pairs (master and three auxiliary mixes). Raw multichannel WAV capture retains its separate 32-input limit.
6. Click **Save setup** to remember the driver, sample rate, buffer and channel masks. Settings are recalled when Device is opened on the next launch. If the saved device cannot be opened, select an available device explicitly. Verify hardware and channel routing before enabling live monitoring.

Focusrite, Behringer WING, Allen & Heath, Dante Virtual Soundcard and other hardware use the same driver-based workflow. Support is contingent on a compatible installed driver and actual hardware testing. Network audio subscriptions and console remote-control functions remain external. Automatic discovery uses JUCE device-list notifications plus a ten-second idle scan, with a one-second notification debounce. Vendor scans are intentionally suspended during live monitoring, playback and recording. Auto reconnect (on by default, user-toggleable) retries the previous device and channel masks when it becomes available, only while idle. Pulse Studio disarms and stops playback/recording on detected loss, rejects unreviewed fallback routing, and never re-arms live audio automatically. Use Configure device to accept a different interface, then manually enable LIVE. Use Add input channels to create muted, named mixer strips for unused enabled input slots in one operation. This is driver-reported discovery, not manufacturer-specific USB/network identification; identical device names may not be unique. Simultaneous independent interfaces are not implemented. Actual hot-plug and reconnection behavior is driver-dependent and requires physical testing.

The Windows CI smoke test checks ASIO backend registration without physical hardware. Review JUCE and applicable ASIO licensing before distributing the binary.

### Driver startup recovery
If a driver fails to start, use the new connection screen to choose the backend, device, sample rate, buffer and initial channel counts before clicking Connect. Selection alone does not open a default device. Start with the driver defaults and 2 inputs / 2 outputs (or a known supported rate and buffer). After connecting, use Advanced settings for individual physical channel masks and the driver control panel. Previously saved configurations are still recalled when first opening Device. The connection report retains the requested settings and the driver's error. The app cannot repair an unavailable driver, missing hardware clock, exclusive access conflict or a driver that does not deliver callbacks. No alternate device is opened by an explicit failed Connect attempt.

## Projects
Version 3 .pulse projects store the variable channel count, routing, ten bus settings, effects, stem paths and plugin state. Older 32-channel projects can be migrated; routes to buses 11–32 are assigned to bus 10, and removed bus settings are not recalled. Projects containing plugins on removed buses are rejected until those plugins are moved or removed in an older version. Save a copy of the original project before migration. Original media and compatible plugins must remain available.

## Build
Install Git, CMake 3.22+, and a C++17 compiler (Visual Studio 2022 with Desktop C++ on Windows; Xcode on macOS). Linux dependencies are listed in .github/workflows/build.yml. Internet is required to fetch JUCE during configuration.

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --parallel 2
    ctest --test-dir build -C Release --output-on-failure

GitHub Actions builds Windows, macOS and Linux, with a Windows installer and native GUI screenshots. Windows builds enable ASIO using the ASIO headers bundled with JUCE 8.0.11. An external compatible SDK may optionally be supplied via -DASIO_SDK_DIR=/path/to/sdk. Dante DVS is separately installed/licensed. Review JUCE and ASIO licensing before distribution.

## Validation
Native regression tests include exact device routing comparisons, simulated driver-fault output silence, bulk creation of 64 input strips, adding 128 mixer channels, deleting a middle channel, verifying stable channel/rack identities, routing audio on bus 10, project restoration, and returning to an empty session. Compilation and hardware verification must be checked for the specific commit. Test with headphones and low levels; never assume production readiness. Plugin latency compensation is not yet connected to the live graph; channel/bus insert effects must report zero latency.
