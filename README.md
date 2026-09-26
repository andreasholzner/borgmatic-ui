# borgmatic-ui

A Qt desktop front end for [borgmatic](https://torsion.org/borgmatic/).

## Build

### Requirements

- CMake ≥ 3.16 and a C++20 compiler (GCC or Clang)
- Qt ≥ 6.6 (Core, Widgets, Concurrent, Test)
- Boost ≥ 1.88 including the compiled Boost.Process library
- cereal, spdlog, nlohmann_json
- Catch2 v3 (tests only)
- trompeloeil (tests only; downloaded automatically if not installed)
- borgmatic at runtime, found on `PATH` (falls back to `/usr/bin/borgmatic`)

On Arch/Manjaro:

```sh
sudo pacman -S cmake qt6-base boost cereal spdlog nlohmann-json catch2
```

On Debian/Ubuntu:

```sh
sudo apt install cmake g++ qt6-base-dev libboost-dev libboost-process-dev \
    libcereal-dev libspdlog-dev nlohmann-json3-dev catch2
```

### Compile

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The executable is `build/borgmatic-ui`. It logs to the terminal at level `info`; set e.g. `SPDLOG_LEVEL=debug` for
more detail.

To build without the tests, add `-DBUILD_TESTING=OFF` to the first command.

### Run the tests

```sh
ctest --test-dir build --output-on-failure
```

## Usage

borgmatic-ui runs borgmatic for one or more borgmatic configuration files, one tab per file. It doesn't create or edit
configurations or repositories itself; set those up with borgmatic first.

### Initializing a new borgmatic backup

The steps below use borgmatic 2.x. They need [borgmatic](https://torsion.org/borgmatic/) and
[borg](https://www.borgbackup.org/); for mounting archives borg also needs FUSE support (`python-llfuse` or
`python-pyfuse3` on Arch/Manjaro, `python3-pyfuse3` on Debian/Ubuntu).

1. Write a configuration file. borgmatic-ui looks for them in `~/.config/borgmatic` by default. A minimal one, e.g.
   `~/.config/borgmatic/home.yaml`:

   ```yaml
   source_directories:
       - /home/me

   repositories:
       - path: /mnt/backup/home.borg
         label: usb-disk
       # or a remote one: - path: ssh://user@host/./home.borg

   # borgmatic-ui runs borgmatic without a terminal, so it can't ask for the passphrase.
   encryption_passcommand: secret-tool lookup borg-repository home

   keep_daily: 7
   keep_weekly: 4
   keep_monthly: 6

   checks:
       - name: repository
       - name: archives
         frequency: 2 weeks
   ```

   `borgmatic config generate -d ~/.config/borgmatic/config.yaml` writes a sample containing every option with
   documentation. If you use `encryption_passphrase` instead of a pass command, make the file readable for you only
   (`chmod 600`).

2. Check the configuration:

   ```sh
   borgmatic config validate -c ~/.config/borgmatic/home.yaml
   ```

3. Create the repository:

   ```sh
   borgmatic -c ~/.config/borgmatic/home.yaml repo-create --encryption repokey-blake2
   ```

4. Export the repository key and keep it somewhere safe, apart from the backup. Without the key and the passphrase the
   backup can't be restored:

   ```sh
   borgmatic -c ~/.config/borgmatic/home.yaml key export --path ~/home-borg-key.txt
   ```

### Working with borgmatic-ui

- **Add a configuration:** *Actions → New Borgmatic Config* (`Ctrl+N`) opens a new tab. Enter the path of the
  configuration file or pick it with the file button next to it. The tab shows the repository location, its size
  and the list of archives.
- **Back up:** *Backup* runs `borgmatic create` followed by `borgmatic check`. With *purge* checked, it runs
  `borgmatic prune` first, which deletes archives according to the `keep_*` options of the configuration. The status
  bar shows borgmatic's progress and then whether the backup succeeded; a failed backup stays visible there.
  *Cancel Backup* interrupts borgmatic.
- **Browse an archive:** select an archive and click *Mount* to choose an empty directory to mount it to. With *Open
  mounted directory* checked, it's opened in the file manager. Mounted archives are highlighted; select one and
  click *Umount* when you're done.
- **Remove a configuration:** *Delete Config* removes the tab. The configuration file and the repository are left
  untouched.

The list of configuration files and the checkbox states are saved when the window is closed.
