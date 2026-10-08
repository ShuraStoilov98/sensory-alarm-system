# Hardware validation and publication checklist

The revised firmware has host behavior tests and a successful ESP32 compile check. Physical checks below remain **pending**. Fill in observed results from the actual assembly before describing the revision as hardware-validated.

## Recorded build check

Validated on 8 October 2026 with placeholder WiFi credentials:

| Item | Version or result |
| --- | --- |
| PlatformIO Core | 6.1.18 |
| Espressif32 platform | 7.1.3 |
| Arduino framework package | 4.20017.260907+sha.dcc1105b, explicitly pinned |
| Arduino core / underlying ESP-IDF | 2.0.17 / 4.4.7, from installed version headers |
| ESP32 toolchain | 8.4.0+2021r2-patch5 |
| Upload tool | esptool 4.11.0, package 2.41100.260830 |
| Target | `esp32dev`, release build |
| Firmware behavior tests | 18 host scenarios passed |
| RAM | 45,076 / 327,680 bytes, 13.8% |
| Flash | 751,553 / 1,310,720 bytes, 57.3% |
| Device upload or bench run | Not performed |
| Hosted GitHub Actions run | Pending push; workflow added, commands verified locally |

The platform and framework pins make the selected platform/SDK explicit. They do not certify the security of every SDK component or freeze all Python/upload-tool dependencies. The underlying ESP-IDF 4.4 branch reached end of life in July 2024, according to the [vendor's final 4.4 release notes](https://github.com/espressif/esp-idf/releases/tag/v4.4.8). This pass preserves the PlatformIO Arduino 2.x baseline; migration to a supported SDK remains a separate build/bench task before treating the device as maintained network-connected firmware.

## Assembly record

| Detail | Observed value |
| --- | --- |
| ESP32 board/module and revision | Pending |
| Driver carrier manufacturer/revision | Pending |
| Carrier sense resistors and measured VREF | Pending |
| Motor part number and rated coil current | Pending |
| Verified coil pairs | Pending |
| PSU rating and measured loaded voltage | Pending |
| Bulk capacitor value/rating/polarity/placement | Pending |
| Printed material and slicer settings | Pending |
| Mechanism, coupling, fasteners and travel distance | Pending |
| Loaded opening/closing duration | Pending |

## Bench checks

Begin with motor power disconnected and follow [Electrical.md](Electrical.md). Perform motor tests unloaded before connecting the curtain. Keep power disconnection accessible; do not intentionally jam the installed mechanism to test a fault.

| Check | Expected behavior | Actual result |
| --- | --- | --- |
| Boot with button released | Driver disabled; no spontaneous movement | Pending |
| Boot with run button held | No movement until released and pressed again | Pending |
| Cold boot without WiFi/NTP | Manual holding still commands travel; releasing stops | Pending |
| Restore WiFi after offline boot | Reconnect requested while idle; NTP service starts after connection | Pending |
| Hold manual button in each direction | Correct physical direction; release stops promptly | Pending |
| Change direction while holding | Movement stops; no reversal until release and new press | Pending |
| Activate each destination limit | Stops promptly and disables driver; held button does not restart | Pending |
| Move away from the opposite active limit | Allowed in the correct direction | Pending |
| Simulate both limits active with motor power disconnected | Fault latched; no further movement commands until restart | Pending |
| Simulate absent destination limit on an unloaded bench motor | Stops at five seconds; held/repeated button presses cannot restart the latched fault | Pending |
| Automatic opening | Begins during scheduled minute when time is valid and no override/fault applies | Pending |
| Press button during automatic travel | Stops; held input does not restart; release then hold selects manual movement | Pending |
| Close manually after automatic opening | Manual closing works on the same day | Pending |
| Restart during the alarm minute after an automatic attempt | Persisted date prevents a second automatic attempt | Pending |
| Manual activity during scheduled minute | Suppresses automatic travel for that day; skipped date saved once idle | Pending |
| Pulse timing and loaded travel | No missed steps/binding; travel fits within timeout with margin | Pending |
| Repeated loaded runs and WiFi loss | Stable movement, reliable end stops, acceptable temperatures | Pending |

For alarm testing, changing the hour/minute in the source does not clear today's persisted attempt. Use a separate commissioning namespace or test device if repeated same-day alarm tests are required; restore the normal namespace and schedule afterwards. Do not routinely erase flash or reset the device to bypass fault inspection.

The daily record is a monotonic calendar date. A clock set incorrectly far into the future can suppress later alarms until that date is reached. Inspect/correct time and the saved record deliberately if this occurs. If a skipped date is awaiting an idle flash write when power fails, that skipped-date record can be lost. Automatic movement always requires a successful write first.

## Human decisions before publication

| Decision or task | Current handling |
| --- | --- |
| Original commit email and CAD paths | Current CAD paths removed; historical copies and personal email preserved. Decide whether this attribution/exposure is acceptable. Changing future Git email settings does not anonymize old commits. |
| Any history anonymization | Requires a separate, coordinated history rewrite. No rewrite or force push has been performed. Preserve contributor credit if removing private metadata. |
| Contributor attribution and ownership | Confirm original collaborator credit is accurate and public attribution is welcome. Confirm the CAD/assets are yours or shared with permission to publish under the repository license. |
| Real installation media | Add a photo and short working demo with permission; inspect room details, screens and photo metadata before committing. CAD previews do not replace operating evidence. |
| Exact hardware and fault behavior | Complete the assembly record and bench checks; confirm the five-second timeout suits the installed travel. Faults require inspection and restart; manual control is hold-to-run, as selected by the owner. |
| Legacy SDK | Decide whether to publish as a documented legacy prototype or migrate to an Arduino/ESP-IDF stack with current support. The existing compile-tested PlatformIO environment still uses ESP-IDF 4.4.7; pinning alone does not resolve SDK vulnerabilities. |
| GitHub publication | Review server-side branches/tags, issues, PRs, Actions logs/artifacts and releases for old private material. Push the reviewed changes, confirm CI, then change visibility if the repository is still private. Visibility has not been changed here. |
| Portfolio placement | Add a concise repository description/topics, pin the project if desired, and link the actual demo. Describe the device as a personal prototype until physical acceptance is recorded. |

See the [public-readiness report](public_readiness_report.md) for the original review and the status of its findings.
