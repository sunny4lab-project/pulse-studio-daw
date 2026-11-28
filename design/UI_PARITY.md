# Approved UI parity contract

The uploaded `Pulse_Studio_UI_Broadcast_Console.html` is the authoritative visual and interaction reference. Preserve it without edits. No page, panel, control, or existing native functionality may be removed merely to simplify implementation. Sample values and preview labels must become truthful engine-backed values and connection states; do not display simulated audio as real.

## Workspace inventory (16)
Dashboard; Arrangement; Mixer; Live Performance; Plugin Manager; Instruments; AI Engineer; Broadcast Control; Routing Matrix; Audio Devices; External Audio; Setlist Manager; Snapshots & Scenes; Mastering; Analytics; Settings.

Preserve the compact dark/teal theme, transport/header, navigation rail, collapsible searchable sound/asset browser, status bar, inspector, timeline, channel colors, fader geometry, banks and scrollable content. Retain existing native workflows and test small and large desktop layouts against the HTML.

## Broadcast must be a dedicated mixer, not a stage view
- Dedicated input device selection and input patch per channel (mono/stereo, shared bus when explicitly selected); add channels; separate input and ten-bus banks; channel color/name, gain, pan, mute, solo, channel selection and insert slots.
- Ten broadcast buses: Drums, Instruments, Vocals, Pastor, FX Reverb, FX Delay, Music, Speech, Ambience, Other. Keep bus routing and processing independent of FOH.
- Broadcast master: source selection, hardware stereo output pair, independent gain, mute, master insert chain, A/V delay; explicit safe arming and routing checks.
- Separate metering and checks view: actual post-master L/R peak, short-term and integrated LUFS, true peak, stereo balance, loudness history, selectable targets, playback profiles, mix balance, signal flow and stream readiness checks. No sample meter values in production.
- Live Performance remains its own stage workspace with setlist, cue launcher, loop/click, FOH/in-ear assignments, DCA controls and safety.

## Native integration status
Initial change in BroadcastPage.h converts existing independent broadcast gain controls to a scrollable vertical console, adds independent pan using the existing engine state, shows the existing input patch and actual combined channel peak, and retains existing mute/lock, mode, scene recall, output arming and change log. It is not yet full parity: existing engine shares the input source/channel processing and uses AUX 1 for broadcast; independent patch, ten broadcast buses, stereo meters, solo, independent inserts, master processing and remaining visual controls still need engine and UI work. The exact HTML remains a preview, not a functional native audio engine.

## Acceptance
Compare screenshots of all 16 workspaces against the reference at desktop resolutions. Verify no controls disappear when resizing (scroll/collapse instead). Test independent broadcast vs stage gain, pan, mute, solo, patch, bus and output isolation, real stereo meters, routing/arming safety, device disconnect, plugin scanning cancellation, recording and scene/project recall. Preserve low-latency realtime constraints: no UI allocations or blocking in audio callback, no automatic output arming, and safe migration of old sessions. Run Windows/macOS/Linux builds and native regression tests before distributing an installer.
