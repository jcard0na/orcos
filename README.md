# ORCOS - OpenRPNCalc Operating System

## Setup


### Build

### Flash

You will need a version of `probe-rs` that supports STM32U385RGTx
At the time of this writing, the main `probe-rs` repo did not support it.
There is a fork included in this repo as a submodule:

```
git submodule sync
git submodule update
make probe-rs
```

Then attach debugger to calculator and

``
make flash
``
