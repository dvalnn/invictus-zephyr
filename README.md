Build project:
```bash
west build -p auto -b <board> invictus/
```

flash project:
```bash
west flash
```

run emulation/simulation:
```bash
west build -t run
```

run tests:
```bash
twister -T invictus/tests/ -G
```
