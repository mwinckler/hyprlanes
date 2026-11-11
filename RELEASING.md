# Releasing Hyprlanes

Hyprlanes is distributed as tagged source releases. Hyprpm builds the plugin against each user's Hyprland headers, and `hyprpm.toml` on `master` selects tested source commits for supported Hyprland releases.

1. Update the version in `CMakeLists.txt` and `src/Plugin.cpp`, along with the release archive URL and compatibility statement in `README.md`.
2. Build from a clean source directory with the commands in `hyprpm.toml`. Verify the resulting plugin against matching Hyprland headers and a running compositor. Check lane creation, horizontal boundaries, vertical focus, sizing, and Omarchy's saved-layout restoration. Validate the Lua examples with `luac -p` and the cycle script with `bash -n`.
3. Commit the release with `jj commit`, then record its full commit hash and tag that commit. Replace `vX.Y.Z` with the release version:

   ```sh
   jj log -r @- --no-graph -T 'commit_id ++ "\n"'
   jj tag set vX.Y.Z -r @-
   ```

4. Read the tested compositor's commit with `hyprctl version -j | jq -r .commit`. In a subsequent commit, add or update its entry under `[repository]` in `hyprpm.toml`:

   ```toml
   commit_pins = [
       ["FULL_HYPRLAND_COMMIT", "FULL_TAGGED_PLUGIN_COMMIT"],
   ]
   ```

   Preserve pins for other supported Hyprland releases. Point each updated pin at a source commit tested with that release. Hyprpm reads the current manifest before checking out the pinned source, so keep its build commands compatible with every pinned commit. Unmatched Hyprland commits use the repository's latest source.

5. Commit the pin update with `jj commit`, advance `master`, and publish the branch and source tag together:

   ```sh
   jj bookmark set master -r @-
   jj git push --remote origin --bookmark master --tag vX.Y.Z
   ```

6. Verify that GitHub's tagged source archive downloads successfully and contains the manifest, source files, license, Lua examples, and cycle script. Check the published pin against the tag's commit and the recorded Hyprland commit.
