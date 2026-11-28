# Pulse Studio Enterprise 0.28.0
Experimental native C++17 / JUCE 8.0.11 application. Not production-ready.

## Expand, compact or hide the navigation sidebar (0.28.0)
Use the small navigation toggle beside the Pulse Studio title in the top bar to
cycle **Expanded → Compact → Hidden → Expanded**. Compact mode shows numbered
workspace shortcuts with full names available as tooltips. Hidden mode gives
the whole sidebar width back to the current mixer, timeline or workspace. The
top-bar toggle always remains visible, even when the sidebar is hidden.
**Ctrl+B** on Windows/Linux or **Cmd+B** on macOS toggles between the last
visible sidebar width and fully hidden, including when a mix workspace is
active. The preference is stored in the application's navigation.state file
and restored at launch independently of the audio project; audio routing and
engine processing are unaffected. When the sidebar is compact/hidden, use the
top toggle to return to Expanded mode for sidebar Save/Load/FX actions.

## Per-channel color coding (0.27.0)
Mixer channel headers and the Broadcast workspace use one shared channel color.
Right-click a channel **name** in Mixer, or its row in Broadcast, to choose
**Automatic by channel role** or one of twelve custom colors. Automatic mode
follows the selected role (speech gold, lead vocal coral, background vocal
purple, drums crimson, bass green, keys/guitar blue); General channels are
colored by bus. Manual colors stay fixed when the role or bus changes. These
settings are saved per channel in project XML and restored on load; older
projects default to Automatic. Audio meters keep their green/amber/red level
indications so channel identity never hides clipping warnings. Channel colors
are display-only and do not affect engine routing or DSP CPU use.

## Independent house and broadcast mixes (0.26.0)
**Broadcast** now offers a separately enabled channel-by-channel stereo feed on
**AUX 1**, with its own broadcast level, mute, pan (recalled in projects), and
automation lock. House channel faders, solos, DCAs and mutes do not change the
broadcast feed's level. Both outputs share the channel's native tone and
compressor DSP but the broadcast tap is before house faders and channel plugin
inserts; it bypasses bus, send, and master effects. A dedicated broadcast
sample-peak limiter operates independently of the house master limiter. AUX 2/3
retain their post-bus behavior. The former AUX 1 post-bus sends are ignored
while independent broadcast feed is enabled.

Configure a *different* stereo hardware output pair for AUX 1 in Routing and
arm it while LIVE is off. Enable the independent broadcast feed while stopped;
then verify both physical outputs before opening the livestream. No AUX output
is automatically armed on startup or on project load. Output availability
requires at least four active physical output channels. An unmapped channel or
a channel whose broadcast fader is at -60 dB contributes no broadcast signal.

Broadcast automation has **Manual**, **Assisted** (monitor-only),
**Autonomous** (bounded gain riding), and **Hybrid** (downward-only gain riding)
modes, with Balanced, Praise, Worship and Sermon profiles, channel-role
targets, peak/noise gating, per-channel lock, and a manual-override latch.
Automation pauses on device/alignment/deadline faults. Four scene slots can
save and recall broadcast fader/pan/mute/lock values and mode/profile without
changing hardware routing or arming. The Broadcast panel records a rolling
100-event in-memory change history. **Scene slots and history are not yet
persisted to project files**, and there is no independent broadcast EQ/plugin
chain, broadcast LUFS metering, true-peak limiter, or hardware-console fader
control. The house LUFS display is specifically labeled as house-only.
Automixing is deterministic and experimental, not a trained AI engineer or a
substitute for supervised real-world soundchecks and broadcast compliance tests.

## Autonomous broadcast fader control (0.25.0)
The top-level mixer now offers **Manual**, **Assisted**, **Autonomous**, and
**Hybrid** modes. In AI Engineer, select a **Balanced**, **Praise**, **Worship**
or **Sermon** profile. All four profiles are lightweight, deterministic gain
policies, not trained AI or automatic recognition of music genres. Assign each
channel's role in Mixer Details (lead vocal, speech, background vocal, drums,
bass or keys/guitar) for role-sensitive targets. When a live speech channel is
active, music and background vocal targets ease down by 3 dB and recover with
a short hold; this does not act as a hard gate or full speech detector.

Autonomous mode adjusts eligible channel faders continuously with a ±0.4 dB
deadband, changes of at most 1.8 dB/second down and 0.6 dB/second up, and a
calibrated anchor range of -6/+3 dB. Hybrid mode only makes reductions;
Assisted mode only suggests levels until Apply is clicked. Channel Lock prevents
automatic changes. Dragging a fader in Autonomous or Hybrid latches a manual
override: unlock that strip to resume automatic control. Automation pauses when
live monitoring is disabled, a channel is muted, solo is active, the master is
near peak overload, the device reports a fault, or the latest callback uses
90% or more of its buffer deadline. Silent inputs do not trigger gain boosts.

This affects Pulse Studio's own mixer, including its broadcast routing; it
does not remotely move Behringer WING FOH faders. It has not been validated
for unattended services or commercial broadcast loudness compliance. Begin
in Assisted mode, with headphones and conservative levels.

## Live performance efficiency and monitoring (0.24.1)
Mapped inputs retain their raw peak meter while muted, solo-excluded, master live-off,
or DCA-muted, but bypass expensive per-sample strip DSP and inserts until audible
again. State resets on the transition to silence to prevent stale filter or fader
history from leaking into the next live activation. This does not reduce CPU used by
unmuted channels or unrelated master/bus processing.

Analytics now shows driver-reported average CPU, the worst measured callback time
relative to its buffer deadline, estimated deadline warnings and recording write
failures. A 100% callback peak means processing took the entire audio-buffer time
budget; a deadline warning indicates a risk of underrun, not definitive proof of
an audible glitch. The counters reset when the audio driver starts, and are not a
substitute for real device, clock, latency or sustained-load validation. The audio
thread updates fixed-size atomic counters with no allocations or logging.

## Mixer
New sessions begin with zero audio input channels and exactly ten renameable stereo summing buses, plus the stereo master. In Mixer, select **+ Audio channel** to add as many audio channels as available resources permit. **Delete channel** opens a channel selection menu. Stop live monitoring, playback and recording before structural edits. New channels are muted and have no assigned input. Open Device to enable physical inputs, then Details to select each mono input or stereo pair. Channels may share a physical input; the number of independent physical signals depends on the interface and driver. Each channel has native processing, a selected bus and an expandable stereo insert rack. Channels, buses and master start with two visible slots and can expand to sixteen. The three auxiliary output pairs are separate from the ten summing buses.

There is no fixed software mixer channel limit. CPU, memory, interface capacity, plugin licenses and device settings constrain practical session size. Legacy four-stem playback and 32-input raw multichannel WAV recording are separate existing subsystems with their own limits.

## Native channel processing and group control
In Mixer, enable Details and select a channel in the channel processing inspector. The native strip now provides eight parametric bell filters, gate, high-band de-esser, HPF and LPF, input trim and polarity inversion, compressor attack/release, transient attack shaping, soft saturation and an optional pre-fader sample-peak clip ceiling. These are lightweight native DSP algorithms, not branded plugin emulations, a true-peak channel limiter or dynamic EQ. All new tone effects default off. The first three EQ bands retain their old settings in migrated projects.

Open **DCA / Tracks** in the mixer toolbar. Assign a channel to one of eight DCA groups, and adjust group level/mute without changing the channel's own fader or its assigned summing bus. DCA groups are gain controls, not additional audio buses.

## Individual track recording
In **DCA / Tracks**, select a mapped channel and enable Record arm, or use Arm mapped to arm all channels with active inputs. Click Record armed WAVs and choose a folder. Pulse Studio creates a new take directory containing separate 24-bit mono or stereo WAVs, named after the channels. Capture is raw hardware audio before trim, processing, DCA and fader; tracks begin on the same callback. The input map is captured when recording starts. Stop via the dialog, Stop inputs in Recording Studio or End rec. The older interleaved recorder remains available separately; the two raw recording modes cannot run concurrently. If an input disappears or the disk FIFO overruns, capture stops and the last block may differ between files; inspect and trim the affected take. Long recordings are subject to WAV size, disk throughput and memory limits. Record arms are intentionally not restored from projects.

## Multitrack arrangement editor
Open **Recording Studio > Timeline**. The advanced editor opens the existing aligned stems as tracks without changing the source until the first committed edit. Import multiple mono/stereo WAV, AIFF, FLAC or supported MP3 files as separate tracks aligned at zero; importing one file adds a clip at the playhead to the selected track, or creates a track if none is selected. Add tracks as needed, select clips, drag horizontally to move, split at the cursor, duplicate or delete. Use the inspector to set source in, length, start, fade in/out, clip gain, track gain/pan, mute and solo, then Apply. Undo and redo retain up to 32 edits during the editor session. All edits are non-destructive and require idle audio. Timeline tracks and clips have no fixed count, subject to memory, file handles, disk throughput and display dimensions. The 48 kHz project clock renders supported 8–384 kHz audio with simple linear interpolation; this is not a high-quality sample-rate conversion or time-stretch engine.

The timeline produces stereo pairs for direct master and individual summing buses, not separate mixer channel inserts. Timeline edits rebuild an immutable read-ahead source; do not edit during live production. Legacy stem imports and setlist selection replace the active advanced arrangement; save the project before switching. Use **Export WAV** to render the arrangement alone, including clip/track processing, to a new 48 kHz 24-bit stereo WAV without connected hardware. Export is cancellable, uses a temporary file, clips samples beyond full scale and excludes live inputs, bus and master effects, master fader and limiter. WAV export is limited to a standard RIFF file below 4 GB. External virtual instruments, pitch shifting, time stretching, high-quality resampling, dedicated track routing and crossfade tools are not yet implemented. Native MIDI notes and gain/pan automation are described below.

## MIDI and track automation
Open Recording Studio > Timeline, add or select an arrangement track, then open **MIDI / Automation**. The piano roll displays 36 pitches at a time and eight-second pages. Choose note length, velocity and time grid; left-click adds notes and right-click removes them. Select Sine, Electric keys, Organ or Pluck (lightweight additive instruments, not sampled instruments or external VST instruments). Select a MIDI input and use Record MIDI / Stop MIDI to capture note-on/off events relative to the current eight-second page's start. Capture is not locked to audio transport, does not monitor incoming MIDI as sound, and does not capture sustain, controllers or pitch bend. Use Import MIDI to merge note events from a standard MIDI file into the selected track at the page start; tempo conversion uses the file's tempo map. Apply to track commits the edit to the arrangement and its undo history.

Gain automation and Pan automation modes display editable envelope lanes. Click to insert/replace a point at the chosen time; right-click removes the nearest point, Clear lane removes all points. Interpolation is linear between points and holds the first/last value outside their range; when a lane is empty, the static track setting applies. These envelopes affect audio clips and native instrument notes during playback and pre-master offline export. All commits require idle audio. Project v7 stores notes, instrument choices and envelopes. There is no independent quantized clip launcher, transport-synchronised MIDI overdub, VST instrument hosting or plugin-parameter automation yet. Arrangement bus routing is available.

## Consolidated production upgrades
Arrangement tracks may target Direct master or any of the ten summing buses using the Timeline inspector. Routed audio reaches bus inserts, gain, shared sends and existing post-bus outputs (aux mixes still require live arming). Compensated mode aligns the arrangement bus input against hardware channel paths and aligns the bus output against the master. The pre-master WAV exporter intentionally ignores these routes and renders the dry track mix. Launch clip seeks the whole arrangement to the selected clip start. Loop clip auditions a selected audio clip alone, repeating at its own length and returning to the arrangement when stopped; it is not a multi-clip quantized session launcher. Playback rate 0.5–2.0 is varispeed (pitch and speed change together); set clip length to fit available media. Independent pitch shifting and pitch-preserving time stretching remain incomplete.

AI Engineer provides opt-in three-band energy analysis, an advisory vocal/instrument midrange masking cut (at most 3 dB in an unused band), and a guarded short-term loudness rider with a selectable -18/-16/-14 LUFS reference, peak headroom and ±3 dB anchor bounds. These are local heuristic DSP features, not a trained AI system. Hybrid mode automatically reduces excessive levels and allows manual application of suggestions; existing automatic level mode remains available. The automatic master rider operates only while the AI Engineer workspace is visible and live audio is safe; it is disabled in Manual mode. No automatic feedback cancellation or guaranteed loudness compliance.

Settings includes opt-in idle autosave every two minutes (suspended during live audio, recording, playback, or device/timeline dialogs), with manual recovery confirmation. Saves use a temporary file and retain one previous .bak copy. Recovery does not replace sessions automatically. Native MIDI capture can begin at the current playhead or page start but still records wall-clock timing and is not a transport-locked overdub or live instrument monitor. External live VST instruments, multi-clip quantized launching, pitch-preserving time stretching, independent pitch shifting, plugin automation, multicore audio scheduling and plugin-process crash isolation remain incomplete.

### External instrument bounce
Select a MIDI-only arrangement track and choose Bounce VSTi. The plugin library can now display installed instruments. Select a compatible stereo instrument (zero audio inputs, stereo outputs, MIDI input), then choose a new WAV path. The application launches a separate helper instance of Pulse Studio to open the plugin in its default state, process note events offline at 48 kHz and write 24-bit stereo WAV with a two-second tail. The rendered track is added with the original MIDI track muted, preserving the original notes. Child process failure or timeout is reported to the host. This is offline hosting, not live VSTi playback, preset/editor control or realtime plugin crash isolation. Plugin scanning itself remains in process. Instrument latency is not compensated in the bounce. Compatible licensed instruments require physical testing.

## Universal Audio Device Manager
1. Connect a compatible USB, Thunderbolt, built-in or network audio interface and install its required operating system driver. Close any other application that exclusively occupies the driver.
2. Open Audio Device Manager and click **Scan all devices** to list registered backends and input/output endpoints. Scanning does not select or open an endpoint for audio streaming. Scan only with live monitoring, playback and recording stopped.
3. Click **Configure device** and select the backend and device. On Windows, ASIO is preferred for professional multichannel audio. Other backends are available as provided by JUCE and the platform. On macOS choose CoreAudio; on Linux choose an available backend.
4. Select sample rate, buffer size and active input/output channels. The mixer displays driver-reported names in active-channel order. Recheck assignments if the active channel mask or device changes.
5. The engine provides four independent stereo mixes (master and three auxiliary mixes); each can be assigned to any distinct consecutive enabled hardware output pair in Routing Matrix. Device setup allows up to 4096 channels per direction, subject to driver capacity. Raw multichannel WAV capture retains its separate 32-input limit.
6. Click **Save setup** to remember the driver, sample rate, buffer and channel masks. Settings are recalled when Device is opened on the next launch. If the saved device cannot be opened, select an available device explicitly. Verify hardware and channel routing before enabling live monitoring.

Focusrite, Behringer WING, Allen & Heath, Dante Virtual Soundcard and other hardware use the same driver-based workflow. Support is contingent on a compatible installed driver and actual hardware testing. Network audio subscriptions and console remote-control functions remain external. Automatic discovery uses JUCE device-list notifications plus a ten-second idle scan, with a one-second notification debounce. Vendor scans are intentionally suspended during live monitoring, playback and recording. Auto reconnect (on by default, user-toggleable) retries the previous device and channel masks when it becomes available, only while idle. Pulse Studio disarms and stops playback/recording on detected loss, rejects unreviewed fallback routing, and never re-arms live audio automatically. Use Configure device to accept a different interface, then manually enable LIVE. Use Add input channels to create muted, named mixer strips for unused enabled input slots in one operation. This is driver-reported discovery, not manufacturer-specific USB/network identification; identical device names may not be unique. Simultaneous independent interfaces are not implemented. Actual hot-plug and reconnection behavior is driver-dependent and requires physical testing.

The Windows CI smoke test checks ASIO backend registration without physical hardware. Review JUCE and applicable ASIO licensing before distributing the binary.

### Driver startup recovery
If a driver fails to start, use the new connection screen to choose the backend, device, sample rate, buffer and initial channel counts before clicking Connect. Selection alone does not open a default device. Start with the driver defaults and 2 inputs / 2 outputs (or a known supported rate and buffer). After connecting, use Advanced settings for individual physical channel masks and the driver control panel. Previously saved configurations are still recalled when first opening Device. The connection report retains the requested settings and the driver's error. The app cannot repair an unavailable driver, missing hardware clock, exclusive access conflict or a driver that does not deliver callbacks. No alternate device is opened by an explicit failed Connect attempt.

## Plugin library and compensation
Open Mixer > Inserts, select a channel, bus or master, then Load effect. Alternatively, Load FX appends to the first free master insert. Scan installed searches the format's standard VST3/AU locations; Scan file supports individual plugin bundles. Search and filter by manufacturer/category or favorites. Each shell sub-effect is stored by its own description. The local catalogue persists across launches. Scanning is in-process and may be slow or crash on incompatible plugins; JUCE's dead-man file helps skip failed scans on restart, not isolate a plugin crash. No VST instruments, sidechains, drag/drop or independent plugin processes yet. Only stereo two-input/two-output effects can be loaded.

Low-latency mode is the default: channel/bus effects reporting nonzero latency are skipped. Enable Compensated mode while stopped to align channel paths at their buses and bus paths with playback at the master. Limits: 4096 samples cumulative per channel/bus rack, 8192 samples alignment. Over-budget active paths silence the output and disarm live processing. Master insert latency is additional, and aux mixes exclude master processing. Runtime plugin latency changes can reset delay buffers; do not change latency-affecting settings during production. No external hardware latency compensation or complete DAW timeline yet. Test with real plugins and hardware.

## Projects
Version 8 .pulse projects store the variable channel count, routing, ten bus settings, expanded insert slots, compensation mode, output pair assignments, effects, stem paths and plugin state. Version 1–7 projects remain readable; version 8 additionally stores arrangement bus destinations. Older builds cannot read version 8. Older 32-channel projects can be migrated; routes to buses 11–32 are assigned to bus 10, and removed bus settings are not recalled. Projects containing plugins on removed buses are rejected until those plugins are moved or removed in an older version. Save a copy of the original project before migration. Original media and compatible plugins must remain available.

## Build
Install Git, CMake 3.22+, and a C++17 compiler (Visual Studio 2022 with Desktop C++ on Windows; Xcode on macOS). Linux dependencies are listed in .github/workflows/build.yml. Internet is required to fetch JUCE during configuration.

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release --parallel 2
    ctest --test-dir build -C Release --output-on-failure

GitHub Actions builds Windows, macOS and Linux, with a Windows installer and native GUI screenshots. Windows builds enable ASIO using the ASIO headers bundled with JUCE 8.0.11. An external compatible SDK may optionally be supplied via -DASIO_SDK_DIR=/path/to/sdk. Dante DVS is separately installed/licensed. Review JUCE and ASIO licensing before distribution.

## Validation
Native regression tests include exact device routing comparisons, simulated driver-fault output silence, bulk creation of 64 input strips, adding 128 mixer channels, deleting a middle channel, verifying stable channel/rack identities, routing audio on bus 10, project restoration, and returning to an empty session. Compilation and hardware verification must be checked for the specific commit. Test with headphones and low levels; never assume production readiness. Consolidated native tests also cover separate shell plugin identities in catalogue XML, 16-slot master and channel layout recall, non-default hardware output pairs and synthetic impulse alignment across the channel/bus/master graph. Added tests exercise DCA/trim/polarity in the graph, eighth EQ band, sample ceiling, separate mono/stereo recordings and invalid input handling. Timeline regression tests cover mixed-rate clip rendering, positioning, fades, splits, solo, project recall, offline export and cancellation. Music tests add native instrument rendering, deterministic seeks, gain/pan automation, invalid MIDI event rejection, project recall and MIDI-only WAV rendering. Production tests cover band analysis, bounded loudness adjustment, bus-specific rendering, stereo export, route recall and backup preservation. These are not tests with commercial plugins or connected physical hardware.
