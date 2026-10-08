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

## Publish the sanitized history

The installed local cleanup was verified across all 19 commits: 30 personal author/committer email fields and two historical CAD path entries were removed. All other CAD archive entries remain byte-identical. Contributor names, dates and messages are preserved; rewritten commit IDs change and existing commit signatures cannot be retained as valid signatures.

The cleaned history is installed locally. Review and commit the media/documentation changes. Select the changed files explicitly, inspect the staged diff, and commit with a descriptive message. Check that your GitHub noreply address is configured; the cleanup sets a noreply alias locally. Use the exact address shown in your [GitHub email settings](https://docs.github.com/en/account-and-profile/how-tos/email-preferences/setting-your-commit-email-address) if GitHub profile attribution requires its numeric ID format.

Publish the cleaned branch before pulling or merging the previous remote history, which would reintroduce its private metadata. The local cleanup records the old remote tip in `.git/privacy-cleanup-lease`. That file is local bookkeeping, not a repository asset. Use its value as an explicit lease when publishing:

```bash
git push --force-with-lease="refs/heads/main:$(cat .git/privacy-cleanup-lease)" origin main:main
```

This updates only `main` and refuses if the server tip changed since the cleanup snapshot. A plain `--force-with-lease` would use the rewritten remote-tracking ref and could reject the intended update. If the explicit lease fails, inspect the new remote work and sanitize it too before retrying. Keep any force-push protection exception limited to this update, then restore it.

Review other server-side branches/tags and any PR references before changing visibility. This local checkout has only `main`; server-side references have not been independently verified. Do not push the entire local ref namespace with `--mirror`, which would also include tool-owned checkpoint refs.

After publishing, confirm GitHub Actions passes on the cleaned branch. Re-clone other local copies rather than merging old history back into the cleaned branch. Forks, old clones and cached GitHub views can retain old data after a branch rewrite; removing every external copy is not something a local Git operation can guarantee. See [GitHub's history-cleanup guidance](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/removing-sensitive-data-from-a-repository) for PR references and support options.

## Final portfolio setup

1. Confirm the repository is public, or change visibility after the cleaned history and CI have been reviewed.
2. Use a concise description, for example: **ESP32 curtain-opening alarm with printed mechanics, offline manual control and fault-aware firmware.**
3. Add relevant topics such as `esp32`, `platformio`, `stepper-motor`, `3d-printing`, `embedded` and `alarm-clock`, then pin the repository if desired.
4. Keep the original collaborator credit and MIT license. The supplied pictures are authorized by the owner's request to use them; no additional filming is required.
5. Describe it as a personal prototype with an earlier installed build and a compile-tested firmware revision. The photo-based layout and CAD animation do not establish that the latest revision has been tested on hardware.

## Future maintenance

The pinned Arduino 2.0.17 / ESP-IDF 4.4.7 baseline is past vendor support; see the [vendor release notes](https://github.com/espressif/esp-idf/releases/tag/v4.4.8). Migrate before describing the device as maintained network-connected firmware. Keep web control, OTA and other optional features outside this release scope.

Complete [hardware acceptance](Validation.md) before making claims about the revised firmware's real pulse timing, current limit, end-stop reliability or loaded travel. Recording those results does not require making a video.
