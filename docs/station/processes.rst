Processes
=========

The Processes tab supervises one configured external command using
``ManagedProcess``, a Qt wrapper around ``QProcess``.

Current behavior:

- Configure command path.
- Configure optional arguments.
- Configure optional working directory.
- Start, stop, and restart the process.
- Show PID, uptime, status, and exit code.
- Display stdout and stderr.

Future process-profile work should add saved profiles for common commands such
as ``goesrecv`` and ``goesproc --mode packet``.
