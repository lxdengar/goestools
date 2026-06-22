Services
========

This page is the canonical service setup guide. The repository currently
contains both legacy system services and newer user-service examples.

User Services
-------------

User services are recommended for per-user installs and build-tree testing.
They live under ``systemd/examples``:

- ``goesrecv.service``
- ``goesproc.service``

Create directories and copy configs:

.. code-block:: sh

   mkdir -p ~/.config/systemd/user ~/.config/goestools ~/goes-data
   cp etc/goesrecv.conf ~/.config/goestools/goesrecv.conf
   cp etc/goesproc.conf ~/.config/goestools/goesproc.conf

Copy and load units:

.. code-block:: sh

   cp systemd/examples/goesrecv.service ~/.config/systemd/user/
   cp systemd/examples/goesproc.service ~/.config/systemd/user/
   systemctl --user daemon-reload

Start and inspect:

.. code-block:: sh

   systemctl --user start goesrecv.service
   systemctl --user start goesproc.service
   systemctl --user status goesrecv.service
   systemctl --user status goesproc.service

Enable for future user sessions:

.. code-block:: sh

   systemctl --user enable goesrecv.service
   systemctl --user enable goesproc.service

To keep user services running after logout:

.. code-block:: sh

   loginctl enable-linger "$USER"

Legacy System Services
----------------------

Legacy Raspberry Pi examples live under ``scripts/services``. They assume
system-wide units under ``/etc/systemd/system`` and paths such as ``/home/pi``.
Use them as older examples, or prefer the user-service files for new setups.

Customizing Paths
-----------------

Adjust service ``ExecStart`` lines to match where binaries and config files
live. For build-tree testing:

.. code-block:: ini

   ExecStart=%h/dev/goestools/build/src/goesrecv/goesrecv -i 10 -c %E/goestools/goesrecv.conf
   ExecStart=%h/dev/goestools/build/src/goesproc/goesproc -c %E/goestools/goesproc.conf -m packet --subscribe tcp://127.0.0.1:5004 --out %h/goes-data

If ``goesrecv.conf`` publishes packets on a different address, update the
``goesproc`` ``--subscribe`` address to match.

Logs
----

View service logs with:

.. code-block:: sh

   journalctl --user -u goesrecv.service -f
   journalctl --user -u goesproc.service -f

``goesproc`` text logs are one event per line when run without a terminal, so
they work directly with journald. To emit machine-readable JSON Lines, add
``--log-format json`` to its ``ExecStart`` command. The default 60-second
summary can be changed with ``--summary-interval SEC`` or disabled with
``--summary-interval 0``.

The interactive packet progress line is automatically disabled for a systemd
service. ``--no-progress`` can be added explicitly when a unit is also used
from a terminal.
