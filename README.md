# exaggerated-shading-demo - An executable

The `exaggerated-shading-demo` executable is a <SUMMARY-OF-FUNCTIONALITY>.

Note that the `exaggerated-shading-demo` executable in this package provides `build2` metadata.


## Usage

To start using `exaggerated-shading-demo` in your project, add the following build-time
`depends` value to your `manifest`, adjusting the version constraint as
appropriate:

```
depends: * exaggerated-shading-demo ^<VERSION>
```

Then import the executable in your `buildfile`:

```
import! [metadata] <TARGET> = exaggerated-shading-demo%exe{<TARGET>}
```


## Importable targets

This package provides the following importable targets:

```
exe{<TARGET>}
```

<DESCRIPTION-OF-IMPORTABLE-TARGETS>


## Configuration variables

This package provides the following configuration variables:

```
[bool] config.exaggerated_shading_demo.<VARIABLE> ?= false
```

<DESCRIPTION-OF-CONFIG-VARIABLES>
