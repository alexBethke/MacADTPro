# Project scripts

## Generate the rainbow resource

The application embeds `rainbow.pict` as a native QuickDraw `PICT` resource.
Generate the resource payload with:

```sh
./scripts/generate_rainbow_pict_resource.sh
```

The script accepts either a standalone PICT with the conventional 512-byte file
header or an already headerless PICT resource. It validates the picture-size
word and writes the resource payload to `src/rainbow.pict`. To use different
paths, pass the input and output filenames:

```sh
./scripts/generate_rainbow_pict_resource.sh input.pict output.pict
```

`./scripts/build.sh` runs the generator before configuring and building the
application, so rebuilding after replacing `rainbow.pict` is sufficient.
