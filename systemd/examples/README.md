# systemd User Service Examples

These example units run `goesrecv` and `goesproc` as systemd user services.
They are intended for per-user installs and build-tree testing; they do not
assume binaries or configs were installed by root.

## Files

- `goesrecv.service` starts the receiver, demodulator, and decoder.
- `goesproc.service` subscribes to the packet publisher and writes processed
  products.

The default examples assume:

- Binaries are in `~/.local/bin`.
- Config files are in `${XDG_CONFIG_HOME:-~/.config}/goestools`.
- Output goes under `~/goes-data`.
- `goesrecv.conf` publishes packets on `tcp://0.0.0.0:5004`, matching the
  sample config in `etc/goesrecv.conf`.
- `goesproc` subscribes locally to `tcp://127.0.0.1:5004`.

systemd expands `%h` to the user's home directory and `%E` to the user's config
directory.

## Setup

Create the directories and copy configs:

```sh
mkdir -p ~/.config/systemd/user ~/.config/goestools ~/goes-data
cp etc/goesrecv.conf ~/.config/goestools/goesrecv.conf
cp etc/goesproc.conf ~/.config/goestools/goesproc.conf
```

Copy the services into the user unit directory:

```sh
cp systemd/examples/goesrecv.service ~/.config/systemd/user/
cp systemd/examples/goesproc.service ~/.config/systemd/user/
systemctl --user daemon-reload
```

Start and inspect the services:

```sh
systemctl --user start goesrecv.service
systemctl --user start goesproc.service
systemctl --user status goesrecv.service
systemctl --user status goesproc.service
```

Enable them for future user sessions:

```sh
systemctl --user enable goesrecv.service
systemctl --user enable goesproc.service
```

To keep user services running after logout, enable linger for the account from a
privileged shell:

```sh
loginctl enable-linger "$USER"
```

That step is optional and depends on how the receiver host is managed.

## Customizing Paths

Edit `ExecStart` in each unit before copying it, or edit the installed copy
under `~/.config/systemd/user`.

For binaries built in this checkout, point `ExecStart` at the build outputs:

```ini
ExecStart=%h/dev/goestools/build/src/goesrecv/goesrecv -i 10 -c %E/goestools/goesrecv.conf
ExecStart=%h/dev/goestools/build/src/goesproc/goesproc -c %E/goestools/goesproc.conf -m packet --subscribe tcp://127.0.0.1:5004 --out %h/goes-data
```

For another install prefix, replace `%h/.local/bin/goesrecv` and
`%h/.local/bin/goesproc` with the actual paths.

If `goesrecv.conf` publishes packets on a different address, update the
`--subscribe` address in `goesproc.service` to match. For a receiver and
processor on the same host, subscribing to `tcp://127.0.0.1:PORT` is usually
appropriate even when `goesrecv` binds to `tcp://0.0.0.0:PORT`.

If you change `WorkingDirectory` or `--out`, create that directory before
starting `goesproc`.

After editing installed units, reload and restart:

```sh
systemctl --user daemon-reload
systemctl --user restart goesrecv.service goesproc.service
```

View logs with:

```sh
journalctl --user -u goesrecv.service -f
journalctl --user -u goesproc.service -f
```
