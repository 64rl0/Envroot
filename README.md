# Envroot

## Purpose
Execute a program in the context of an arbitrary “environment root” via the special `#!...` (shebang) line.
For example, if you want to execute `python3 --version`, but you only know the path to the python3 interpreter at runtime.

## How does envroot solve these problems?
The envroot command is very simple. All it does is expand variables using the current environment.
- Accepts a single shebang template argument (like `$PYTHONBIN` or `$ENVROOT/bin/python3.12`).
- Expands `$VAR` and `${VAR}` using the current environment.
- If the variable value is a colon-separated list and a suffix is present, searches each entry for an executable match.
- Executes the resolved interpreter and forwards the original script and arguments unchanged.

## Usage
Basic (direct interpreter variable):
```sh
#!/icarus/bin/envroot "$PYTHONBIN"
```
```sh
#!/icarus/bin/envroot "$ENVROOT/bin/python3.12"
```

Search in a list variable (first matching entry wins):
```sh
#!/icarus/bin/envroot "$PYTHONPATH/bin/python3.12"
```

## Behavior and limits
- Shebangs pass only a single argument to the interpreter; envroot treats that as the template.
- Quotes in the shebang are literal; envroot strips `"` and `'` before expansion.
- If the resolved path is missing or not executable, envroot exits with an error.

## Platform notes
- The kernel requires the shebang interpreter must be a binary. Use the compiled envroot binary (not a shell script).
- If you want to use envroot on macOS, then you need to know that `#!` line parsing is done differently. The effect of this is that you should always make your `#!` line contain a single argument.
- `$VAR` is expanded inside `/icarus/bin/envroot`, not by the kernel.
- Single quotes and double quotes on the shebang `#!` line are passed to `/icarus/bin/envroot`.
