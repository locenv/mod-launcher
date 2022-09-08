# A module to launch native binary in the background

## Installation

```sh
locenv mod install github:locenv/mod-launcher
```

## Usage

```lua
local l = require 'launcher'
local p = l.spawn('path/to/binary')
```

## API

### launcher.spawn (binary [, wd])

Launch `binary` in `wd` and return the `process` object immediately without waiting for the process
to exit. If `wd` is absent it will launch in the working directory of the script's current context.

The process will force exit if it is still running when the `process` object is out of scope.

### process:wait ()

Wait for the process to exit.

## License

MIT
