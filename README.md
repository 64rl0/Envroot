# Envroot

## Purpose
Execute a program in the context of an arbitrary “environment root” via the special `#!...` (shebang) line.
For example, if you want to execute `python3 --version`, but you only know the path to the python3 interpreter at runtime.

## How does envroot solve these problems?
The `envroot` command is very simple. All it does is locate a special file named `.envroot` somewhere up the directory structure from where your script is executed, The directory containing the special file becomes the `ENVROOT` environment variable.
Next `envroot` expands variables using the current environment.
- If `OVERRIDE_ENVROOT` is set and non-empty, `envroot` uses it as `ENVROOT` and skips `.envroot` discovery.
- If `OVERRIDE_ENVROOT` is unset, before expanding variables, it starts from the script path (`argv[2]`) and walks up parent directories looking for `.envroot`.
- If found, it exports `ENVROOT` as the absolute path of the directory containing `.envroot`.
- If `.envroot` is not found before reaching `/`, it exits with an error.
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

## I want to run a script in a different environment
The best thing to do is to create a symlink to your script “inside the environment”. However, you can force `envroot` to use a specific environment root by setting a non-empty `OVERRIDE_ENVROOT` environment variable. Be careful with this as all child processes will also inherit this environment variable which often will give you undesired side effects. Basically, you only want to use this behavior in very exceptional situations.

## Behavior and limits
- Shebangs pass only a single argument to the interpreter; `envroot` treats that as the template.
- Quotes in the shebang are literal; `envroot` strips `"` and `'` before expansion.
- If the resolved path is missing or not executable, `envroot` exits with an error.

## Platform notes
- The kernel requires the shebang interpreter must be a binary. Use the compiled `envroot` binary (not a shell script).
- If you want to use `envroot` on macOS, then you need to know that `#!` line parsing is done differently. The effect of this is that you should always make your `#!` line contain a single argument.
- `$VAR` is expanded inside `/icarus/bin/envroot`, not by the kernel.
- Single quotes and double quotes on the shebang `#!` line are passed to `/icarus/bin/envroot`.
