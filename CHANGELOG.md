# Changelog

All notable changes to HowsTheWeather will be documented in this file.

## [v1.4.15] - 2026-05-07

### Fixed
- **Position knob amber-arc indicator now appears.** `setupKnob("knob-position", …)` was not being assigned into `knobInstances`, so `updateModulatedKnobs()` could never reach it. Now registered like the other modulated knobs.
- **Pitch amber-arc now shows the real modulation depth.** The pitch offset was being divided by the knob's full range (`/ 48`) before being passed to `updateUI`, but `updateUI` adds the offset to the base value in *actual units* — so the arc was being shifted ~48× less than it should have. Pass `pitchOffsetSt` directly.
- **Negative modulation offsets are now visible.** The amber arc was drawn under the white base arc, so any modulation that pulled the value below the base was hidden behind it. Reworked `drawKnob` to draw the white base arc first, then the amber arc as a *difference segment* between the base and modulated angles on top — visible regardless of sign.

### Implementation
- `Source/ui/public/index.html` — three small JS changes: knob registration, pitch updateUI argument, and `drawKnob` arc draw order.

### Testing
- Build SUCCESS — AU + VST3 + Standalone built and auto-installed.

## [v1.4.14] - 2026-05-07

### Changed
- Weather modulation re-routed: **Temperature → Position** (replaces previous Temperature → Feedback). Feedback knob no longer receives weather modulation.
- Pressure → Pitch is unchanged.

### Implementation
- `Source/PluginProcessor.cpp` — temperature offset now applied to `positionVal` with the same `(norm - 0.5) * 1.2 * amount` scaling (max ±0.6, clamped to 0–1); feedback offset block removed.
- `Source/ui/public/index.html` — `updateModulatedKnobs()` now drives the position knob's amber-arc indicator from `weatherNorms.temp`; feedback knob arc is cleared. Info-panel "Weather Mappings" text updated.

### Notes
- Parameter IDs unchanged → existing presets still load.
- The orchestrator-owned contracts (`handoff_artifacts/PLUGIN_SPEC.md`, `PARAMETER_SPEC.md`, `PARAMETER_REGISTRY.md`) still describe the old Temperature → Feedback mapping; rerun `/orchestrate` if you want the contracts realigned with the implementation.

### Testing
- Build SUCCESS — AU + VST3 + Standalone built and auto-installed to `~/Library/Audio/Plug-Ins/`.

## [v1.4.13] - 2026-05-07

### Fixed
- Drawer no longer leaves a wide empty gap to the right of the Weather "Amount" knob — the three sections (Grain Field, Cloud Tail, Weather) now distribute evenly across the full drawer width
- Preset bar items (select / prev / next / Save) now center as a group between the Freeze and Close buttons instead of stretching the dropdown across the full bar
- Restored breathing room between knobs within each section (drawer-knobs gap back to 6px after v1.4.12 had tightened it to 4px)

### Change (CSS-only, `Source/ui/public/index.html`)
- `.drawer-content`: replaced `gap: 6px` with `justify-content: space-between` — fills the available width by spacing the 3 sections + 2 dividers evenly
- `.drawer-knobs` gap: 4px → 6px (restored)
- `.preset-bar`: added `justify-content: center`; gap 6px → 4px
- `.preset-select` and `.preset-input`: replaced `flex: 1` with `flex: 0 0 auto; width: 150px` so the dropdown/input no longer stretch and the preset bar items stay packed in the centre
- No HTML structure, window size, or positioned-element changes

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.4.12] - 2026-05-07

### Fixed
- Weather "Amount" knob no longer clipped off the right edge of the parameters drawer. Was visible in earlier versions, then started overflowing once total drawer content exceeded the 600px-wide window.

### Root cause
- 600×600 plugin window with default drawer gaps (`drawer-content` gap: 10px, `drawer-knobs` gap: 6px) made the row "Grain Field (4 knobs) | divider | Cloud Tail (5 knobs) | divider | Weather (1 knob)" narrowly overflow, so the rightmost Weather section was clipped by `body { overflow: hidden }`.

### Change (CSS-only, `Source/ui/public/index.html`)
- `.drawer-content` gap: 10px → 6px (saves ~16px across 4 inter-section gaps)
- `.drawer-knobs` gap: 6px → 4px (saves ~14px across the two knob rows)
- No change to window size, knob-container width (still 46px), knob canvas size (still 36×36), HTML structure, or any positioned elements (freeze button, params button, info toggle, preset bar, globe, title — all anchored or auto-centered, untouched)

### Testing
- Build SUCCESS, AU + VST3 installed (timestamps 2026-05-07 17:37)

## [v1.4.11] - 2026-05-05

### Restored
- 2D rain dot particles (as in v1.4.10 and earlier). `#weather-fx` canvas, `fxCanvas`/`fxCtx`, `rainDrops` array, and the rain populate/depopulate + draw passes are back at z-index 1 (above clouds, below globe)
- `fxWeather.rain` / `fxSmooth.rain` smoothing and `fxWeather.rain = rainAmount` in the `weatherUpdate` listener

### Kept removed
- Wind streak lines (no `windStreaks` array, no streak draw pass)

### Confirmed working from v1.4.7
- Cloud shader advection follows API wind: `cp += u_windVec * u_time * 0.25`, where `u_windVec = unitDir(angle+180°) * windMagnitude`. No wind → no cloud motion
- Cloud cover gate: `threshold = mix(1.05, 0.32, u_cloud)` plus `cloud *= smoothstep(0.04, 0.15, u_cloud)` — clear sky stays clear when API reports near-zero cloud cover

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.4.10] - 2026-05-05

### Removed
- 2D rain dot particles
- 2D wind streak lines
- `#weather-fx` canvas element + associated CSS
- `fxCanvas`, `fxCtx`, `resizeFxCanvas`, `rainDrops`, `windStreaks`, `drawFxParticles` and the rAF draw loop

### Kept
- Cloud shader (procedural fBM clouds + sun halo) — sole weather visual now
- Wind direction smoothing + cloud cover smoothing — still feed the shader uniforms
- Rain text in info area (e.g., "1.2 mm rain") still reads `data.rain` directly

### Notes
- `updateFxState()` rAF loop replaces the old draw loop: just smooths `fxSmooth.{wind, cloud, windDir}` for the shader
- `fxWeather`/`fxSmooth` lost the `rain` field (no longer consumed)

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.4.9] - 2026-05-05

### Changed
- Sun moved to dead-center `(0.5, 0.5)` — directly behind the opaque globe. The core is occluded by the globe; only the soft halo bleeds out around the silhouette, creating a backlit-rim lighting effect during daytime
- Glow radius extended `0.30 → 0.55` and intensity raised `0.28 → 0.45` so the halo reaches past the globe edge
- Core dimmed (`0.7 → 0.4×`) since most of it is hidden anyway

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.4.8] - 2026-05-05

### Fixed
- Wind streak particles now drift in the actual API wind direction (matches the cloud shader). Previously the streaks moved hardcoded left-to-right and the orientation of each line was horizontal regardless of wind data — leftover from v1.2.0
- Streaks rotate so the line points in the wind-flow direction; respawn on the upwind edge (whichever axis is dominant)

### Root Cause
- `drawFxParticles()` only added `s.speed` to `s.x`. v1.4.7 added `wind_direction_10m` to the data pipeline for the cloud shader but never wired it to the 2D-canvas streaks

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.4.7] - 2026-05-05

### Added
- Wind direction added to weather pipeline. Open-Meteo `wind_direction_10m` is now fetched, stored as `rawWindDirection_` in `WeatherService`, exposed via `getWindDirection()`, and pushed to JS as `windDirection` on every `weatherUpdate` event

### Changed
- Cloud advection now follows actual wind direction. Shader takes a `u_windVec` (unit direction × wind magnitude); no wind = no motion. Direction = `(meteo angle + 180°)` so clouds visibly travel TOWARD the destination, with 0°=north (top→bottom), 90°=east (left→right). Smoothed in JS via shortest-arc lerp around the 360° wheel
- Clouds now hard-gated by API cloud cover: hidden when cover < ~5%, fade in between 5% and 15%. Threshold raised so even at low cloud cover the sky stays clear
- Sun moved from upper-right to top-center (no longer behind the "What's This?" button); core dimmed (0.07→0.04 radius, 0.7× brightness) and glow radius/intensity reduced (0.45→0.30 radius, 0.6→0.28 intensity)

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.4.6] - 2026-05-05

### Changed
- Cloud visualization upgraded from a static CSS radial gradient to a custom **WebGL fragment shader** drawing animated procedural clouds + sun + sky. No external libraries (after the VANTA.JS / three.js attempt in the reverted v1.4.5 broke the WebView).

### Implementation
- New `#cloud-canvas` (z-index 0, behind everything) runs a single-pass fullscreen fragment shader
- Shader: 5-octave fBM noise (value-noise + smoothstep), advected by `u_time` + `u_wind`; soft sun disk + glow at upper-right; vertical sky gradient from horizon to top; cloud cover threshold driven by `u_cloud`
- Time-of-day state (existing `bgCurrent` HSL) drives `u_skyTop`, `u_skyHorizon`, `u_cloudColor`; `u_dayFactor` (computed from `bgCurrent.light1`) fades the sun warm-orange (day) → faint blue-gray (night)
- Wind drives cloud scroll speed and sun warmth; cloud cover drives density threshold
- Self-contained IIFE at the end of the module script — if shader compile/link fails, the rest of the UI continues to work and the body gradient still shows

### Removed
- `#cloud-haze` element + its `radial-gradient` updates in `updateFxParticles()`

### Layering
- `#cloud-canvas` at z-index 0 (background)
- `#weather-fx` at z-index 1 (rain/wind particles, in front of clouds)
- `#globe-wrap` at z-index 5 (in front of FX)
- Panels at 15, buttons/title at 20

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.4.5] - 2026-05-05 *(reverted)*

VANTA.JS Clouds + three.js integration. Caused the WebView to render only the COBE globe; reverted via commit `67a496e`. See v1.4.6 for the working replacement.

## [v1.4.4] - 2026-05-05

### Fixed
- Preset bar no longer stays visible when switching from Params drawer to What's This panel. Info-toggle click handler now also strips `.visible` from `#preset-bar` (was only closing the drawer + toggle text)
- Info panel content (bug-report textarea + Send/Cancel) no longer cut off. `max-height` raised from `70%` to `calc(100% - 80px)` — leaves room for Freeze/Close while letting the panel grow as the bug form opens, then shrink back when "Thanks for feedback!" closes after 3s. Resize is automatic via natural flex/auto-height

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.4.3] - 2026-05-05

### Changed
- Bug Report Send now actually delivers email via Web3Forms HTTPS POST → `frankyredente@gmail.com`. Reports include plugin version + timestamp
- Removed local-file capture (no more files in `~/Library/Application Support/Franky/HowsTheWeather/BugReports/`)
- POST runs on a `juce::Thread::launch` background thread (5s timeout); UI shows "Thanks for feedback!" immediately, no blocking

### Notes
- Free Web3Forms tier: 250 submissions/month
- Offline → report silently dropped (UX identical to online; user already saw "Thanks")
- Web3Forms access key embedded in source — if leaked, rotate at web3forms.com

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.4.2] - 2026-05-05

### Fixed
- Preset bar no longer overlaps drawer knob row. Drawer bottom-padding restored from 14px → 52px (the gap-closing change in v1.4.1 was wrong: knobs extended into the preset bar's area)
- Bug Report no longer hides the plugin UI on Send. Replaced unreliable `mailto:` (which never auto-sends and can hijack the WebView) with a local-file write — reports are saved to `~/Library/Application Support/Franky/HowsTheWeather/BugReports/bug_YYYY-MM-DD_HH-MM-SS.txt`

### Added
- "Thanks for feedback!" inline confirmation appears for 3 seconds after Send
- New native function `submitBugReport(body)` (replaces `openBugMail`)

### Root Cause
- Drawer overlap: v1.4.1 reduced bottom-padding to "close the gap" without realizing the knob row needs the 52px reserve to clear the bottom-row buttons
- Bug-report behavior: `mailto:` only opens a draft in the user's mail client, never auto-sends. User expected a confirmation + delivered email — neither happened. Local-file capture is reliable, immediate, and visible

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.4.1] - 2026-05-05

### Fixed
- Bug-Report Send no longer breaks the plugin UI. Mailto now launched from C++ via `juce::URL::launchInDefaultBrowser()` instead of `window.location.href = "mailto:..."` — the WebView no longer tries to navigate into the unsupported URL
- Output distortion: added `tanh` soft-clip on final output to prevent harsh clipping when feedback + reverb + dry/wet stack high

### Changed
- Preset bar moved out of the drawer to body level — sits between Freeze and Close pill buttons (`bottom: 14px; left: 100px; right: 100px`), only visible while drawer is open
- Preset bar order: dropdown → ◀ → ▶ → Save (arrows adjacent, Save next to arrows)
- Drawer bottom-padding reduced 52px → 14px (closes the gap above the preset bar)
- New native function `openBugMail(body)` registered

### Root Cause
- mailto:: WebView treats `mailto:` as a navigation target and renders an "unsupported URL" error page in place of the plugin UI. Native code uses the OS URL handler instead
- Distortion: no output stage limiter; granular feedback path could produce values >1.0 which then digital-clipped

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.4.0] - 2026-05-05

### Added
- Preset bar in params drawer: prev/next arrows, dropdown of saved presets, Save button (prompts for name)
- Native C++/JS bridge for presets: `savePreset`, `loadPreset`, `nextPreset`, `prevPreset` exposed via `Juce.getNativeFunction`
- `presetUpdate` event emitted from C++ timer with current preset name + full list
- Bug Report section in info panel: "Report Bug" button reveals textarea + Send/Cancel; Send opens default mail client via `mailto:frankyredente@gmail.com` with subject "Bug Report"

### Changed
- Removed duplicate `<h3>How's The Weather</h3>` from info panel (title already shown elsewhere)

### Wiring
- `PluginProcessor` now owns a `PresetManager` member exposed via `getPresetManager()`
- Presets stored at `~/Library/Application Support/Franky/HowsTheWeather/Presets/*.xml`

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.3.9] - 2026-03-23

### Changed
- Rain dots enlarged: radius 3–4.5px (was 1.5–2px), opacity boosted +0.3 for better visibility

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.3.8] - 2026-03-23

### Added
- Weather code fallback for rain detection — when API precipitation reads 0 but weather_code indicates drizzle (51-55), rain (61-65), showers (80-82), or thunderstorms (95-99), a minimum rain amount is applied
- Fallback rain amounts scale by intensity: drizzle 0.2mm, light rain 0.5mm, heavy rain 2.0mm, showers 1.0-3.0mm, thunderstorms 2.0mm
- Rain dots animation now triggers from weather_code even when precipitation measurement is 0
- weather_code sent from C++ to WebView for JS processing

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.3.7] - 2026-03-23

### Fixed
- Switched weather API from `rain` to `precipitation` field (combines rain + showers + snowfall) for more accurate rain detection
- Added `weather_code` to API request for future use
- UI text still displays as "rain" / "No rain"

### Root Cause
- Open-Meteo `rain` field only captures steady rain, missing showers (convective rain) and other precipitation types

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.3.6] - 2026-03-23

### Fixed
- Rain info now always shown in weather text — displays "No rain" when rain is 0, or "X.X mm rain" when raining
- Rain dot animation was already correctly wired (API → C++ → WebView → dots) — only the text display was suppressing zero-rain

### Root Cause
- Line 741: rain text condition `data.rain > 0` returned empty string when rain was 0, making it look like rain data wasn't being gathered

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.3.5] - 2026-03-23

### Changed
- Title ("How's The Weather") and "What's This?" button now absolutely positioned with top: 14px (matches Freeze/Params bottom: 14px margin)
- Globe moved up: title no longer pushes globe down in flex flow
- Globe top padding independently controllable via `#globe-wrap { padding-top: 10px }`
- Weather data text decoupled from globe — now absolutely positioned at `#info-area { bottom: 48px }`, moves independently
- Globe sizing uses 90px height reserve (was 120px)
- UI is now fixed size (600x600), no longer resizable

### Testing
- Build SUCCESS, AU + VST3 installed

## [v1.3.4] - 2026-03-21

### Fixed
- Reverted globe size back to original (120px reserve, was incorrectly changed to 240px)
- Full clean rebuild ensuring both AU and VST3 get updated binaries

### Testing
- Build SUCCESS, all formats (VST3, AU, Standalone)

## [v1.3.3] - 2026-03-21

### Fixed
- Globe sized smaller (reserves 240px for drawer area, was 120px) so drawer no longer covers it
- Drawer knobs have more room above Freeze/Close buttons (52px bottom padding)

### Testing
- Build SUCCESS, all formats (VST3, AU, Standalone)

## [v1.3.2] - 2026-03-21

### Fixed
- Globe, location label, and weather info positioned higher in the UI
- Params/Close button stays at same position when toggling (removed translateY shift, added min-width)

### Testing
- Build SUCCESS, all formats (VST3, AU, Standalone)

## [v1.3.1] - 2026-03-21

### Changed
- Globe shifted slightly higher for better balance when drawer is open
- Freeze and Close/Params buttons aligned to same height (bottom: 14px)
- Drawer knobs positioned higher with more breathing room (padding 12px top, 46px bottom)

### Testing
- Build SUCCESS, all formats (VST3, AU, Standalone)

## [v1.3.0] - 2026-03-21

### Changed
- Params drawer: Grain Field and Cloud Tail side by side with thin dividing lines, Weather Amount separated by divider on the right
- Params drawer shorter, does not overlap Freeze button when open
- Globe sphere color: light blue by default, turns white when Freeze is active
- Rain particles changed from lines to dots/points for better visibility
- Wind streaks more visible: longer (20-60px), higher opacity (0.4-0.65), more streaks (max 70), lower threshold (0.1)

### Testing
- Build SUCCESS, all formats (VST3, AU, Standalone)

## [v1.2.1] - 2026-03-21

### Changed
- Default parameter positions from user preset: position 0.10, size 0.93, density 0.83, texture 0.87, pitch -4.6 st, feedback 0.70, drywet 1.00, spread 0.82, reverb 0.32, weather_amount 1.00
- Weather modulation always active (removed DSP toggle button)
- Params drawer more compact: smaller knobs (36×36), Weather Amount moved to top-right corner
- WeatherService slew rate increased from 2 Hz to 30 Hz (coefficient 0.007) for smooth transitions

### Fixed
- Weather particle effects not visible: increased opacity, lowered thresholds, added devicePixelRatio canvas scaling
- Stepped/audible modulation jumps when dropping a pin — now smoothly interpolates over ~5 seconds

### Root Cause
- Particles: opacity values too low (rain 0.15→0.3, wind 0.08→0.15), thresholds too high, no HiDPI canvas support
- Modulation steps: 2 Hz slew timer meant audio thread saw 500ms staircase updates; 30 Hz eliminates audible stepping

### Testing
- Build SUCCESS, all formats (VST3, AU, Standalone)

## [v1.2.0] - 2026-03-21

### Added
- Weather particle effects: rain drops (vertical falling dots), wind streaks (horizontal lines), cloud haze (radial fog overlay)
- Freeze button moved outside params drawer as pill-btn (bottom-left corner)
- Rain data from Open-Meteo API displayed in weather info text

### Changed
- Default parameter values: dry/wet 0.7, feedback 0.3, reverb 0.1 (plugin is audible from first launch)
- Weather modulation depth doubled: 0-1 params now ±0.6 max (was ±0.3), pitch ±12 st (was ±6)
- Freeze button removed from params drawer (now standalone bottom-left)
- Info panel updated with visual effects and controls documentation

### Testing
- Build SUCCESS, all formats (VST3, AU, Standalone)

## [v1.1.0] - 2026-03-20

### Added
- "What's This?" info panel (top-right button) explaining how the plugin works
- Time-of-day background: color reflects solar time at the selected location (amber sunrise, light blue daytime, orange sunset, dark navy night)
- Weather-modulated knobs show amber overlay arc when DSP modulation is active
- Knobs for affected parameters (size, density, pitch, feedback, texture) move in real-time with weather data

### Changed
- Density, feedback, and reverb parameters now use logarithmic (skew 0.5) scaling for finer control at low values
- Removed GUI toggle button from params drawer (weather GUI visuals are always on)
- Background no longer driven by weather data; instead uses real-world solar time calculation based on selected longitude

### Testing
- Build SUCCESS, all formats (VST3, AU, Standalone)

## [v1.0.2] - 2026-03-19

### Changed
- Default plugin window size set to 1148×1042 pixels (resizable, limits 600×500 to 2400×1800)
- Renamed "Main Controls" section to "Grain Field"
- Renamed "Secondary" section to "Cloud Tail"
- Section boxes more transparent (35% white + backdrop blur) for better background visibility
- Default background changed from flat grey to vibrant multi-color gradient (purple/magenta/coral/amber)
- Weather-reactive background now uses 3-stop gradient (sky/atmosphere/ground) with wind-driven angle

## [v1.0.1] - 2026-03-19

### Fixed
- Toggle buttons (GUI, DSP, Freeze) now respond to clicks
- UI fits within plugin window without scrollbar
- Lat/Lon inputs update in real-time while typing (was requiring Enter key)
- Weather status text clarified ("Fetching weather data from Open-Meteo...")

### Root Cause
- Wrong JUCE 8 JS API for toggles: `getToggleButtonState()` → `getToggleState()`, `getNormalisedValue()` → `getValue()`
- UI overflow from generous padding with `overflow-y: auto`
- Coord inputs used `change` event instead of `input` event

## [v1.0.0] - 2026-03-19

### Added
- Granular DSP engine (16-bit circular buffer, 32-grain pool, allpass diffusion, FDN reverb)
- 15 automatable parameters (10 granular, 5 weather modulation)
- WebView GUI with canvas knobs, weather particle background, routing bars
- Open-Meteo weather API integration with slew-limited modulation
- VST3 + AU formats
