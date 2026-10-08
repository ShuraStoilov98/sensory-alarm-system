# Publication decisions and remaining actions

The intended release is a documented personal hardware prototype. Filming a working demo is outside scope. The revised firmware has 22 passing behavior scenarios and an ESP32 compile check; physical acceptance is still pending and is disclosed in the README.

## Decisions made

| Item | Decision |
| --- | --- |
| Historical privacy | Remove personal author/committer emails and historical CAD source-file paths. Preserve contributor names, original dates, commit messages, source files and geometry. |
| Lead installation image | Use the bright controller close-up, screenshot 1. It clearly shows the ESP32, driver, controls and printed mounting. |
| Supporting image | Use screenshot 2 for the curtain mechanism. Its low-light appearance is retained; it provides installation context rather than fine mechanical detail. |
| Alternate images | Omit screenshots 3 and 4; they repeat the controller view with poorer exposure. |
| Image processing | The chat screenshots were not available as local files. The stored contact sheet is generated from those references and labeled as an AI-assisted layout. It is not an untouched screenshot export. |
| Video | Provide a six-second [CAD turntable](images/cad-turntable.mp4), with an [inline GIF](images/cad-turntable.gif). It renders the original 3MF geometry and is explicitly labeled as CAD, not operating footage. |
| Release scope | Publish the current stack as a documented legacy prototype. A supported SDK migration and bench acceptance are future maintenance work, not prerequisites for sharing prototype source honestly. |

## Published repository and future pushes

The cleaned history was published on 8 October 2026 to the independent [sensory-alarm-system repository](https://github.com/ShuraStoilov98/sensory-alarm-system). The initial upload contains 20 commits through `844dc80`, including the final presentation assets. The owner created this destination as public. The original repository's `main` was also updated successfully with an explicit force-with-lease.

The cleanup covered the preceding 19 commits: 30 personal author/committer email fields and two historical CAD path entries were removed. All other CAD archive entries remain byte-identical. Contributor names, dates and messages are preserved; rewritten commit IDs change and existing commit signatures cannot be retained as valid signatures. A follow-up check of all 20 commits on the published branch found no personal email fields or private Windows paths in CAD archives; the checkout secret scan found no credential candidates.

The local `public` remote points to `https://github.com/ShuraStoilov98/sensory-alarm-system.git`, and `main` tracks `public/main`. Future normal updates use:

```bash
git push
```

Push only reviewed branches. Do not publish the entire local ref namespace with `--mirror`, which could include tool-owned checkpoints or old history. The `.git/privacy-cleanup-lease` file records the completed original-repository update; it is local bookkeeping and is no longer the publication procedure.

Keep the original repository private. Re-clone other local copies from the new repository rather than merging previous history into the cleaned branch. An earlier pull fetched the old history locally but stopped before merging; the published `main` remains clean. A new repository isolates this release from the original repository's PR references and cached views, but does not erase old clones, forks or previously exposed copies. See [GitHub's history-cleanup guidance](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/removing-sensitive-data-from-a-repository).

Confirm [GitHub Actions](https://github.com/ShuraStoilov98/sensory-alarm-system/actions) passes on the published branch. The push succeeded; hosted CI and the rendered public README have not been independently verified in this pass. Keep a GitHub noreply address configured for future commits; use the exact address in your [GitHub email settings](https://docs.github.com/en/account-and-profile/how-tos/email-preferences/setting-your-commit-email-address) if profile attribution requires its numeric ID format.

## Final portfolio setup

1. Check the rendered README, images and diagrams in the public repository, and confirm GitHub Actions passes.
2. Use a concise description, for example: **ESP32 curtain-opening alarm with printed mechanics, offline manual control and fault-aware firmware.**
3. Add relevant topics such as `esp32`, `platformio`, `stepper-motor`, `3d-printing`, `embedded` and `alarm-clock`, then pin the repository if desired.
4. Keep the original collaborator credit and MIT license. The supplied pictures are authorized by the owner's request to use them; no additional filming is required.
5. Describe it as a personal prototype with an earlier installed build and a compile-tested firmware revision. The photo-based layout and CAD animation do not establish that the latest revision has been tested on hardware.

## Future maintenance

The pinned Arduino 2.0.17 / ESP-IDF 4.4.7 baseline is past vendor support; see the [vendor release notes](https://github.com/espressif/esp-idf/releases/tag/v4.4.8). Migrate before describing the device as maintained network-connected firmware. Keep web control, OTA and other optional features outside this release scope.

Complete [hardware acceptance](Validation.md) before making claims about the revised firmware's real pulse timing, current limit, end-stop reliability or loaded travel. Recording those results does not require making a video.
