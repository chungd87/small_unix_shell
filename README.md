# Custom Unix Shell
- Created shell with support for running background and foreground processes,
overriding SIGSTP signal for usage as the toggle.
- Manually built commands for changing directory, status, and input/output
redirection, with other commands being sent to exec() function family.
- Handled memory management and tracking of forked child processes for proper
termination.
