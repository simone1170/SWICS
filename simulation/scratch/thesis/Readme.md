# SWICS

## Running the testbed

### Command-line arguments

- `use5g`: boolean indicating the choice of network configuration (default: `true`)
- `createlogfiles`: boolean indicating whether to create log files (pcap, physical state, ...) of simulation run (default: `true`)
- `loggingLevel`: argument indicating the ns-3 component logging level (default: `INFO`)
- `loggingGroup`: argument indicating which components of predefined groups should log to the console (default: `ALL`)
- `includeLoggingComponents`: comma-separated list of logging components to be enabled
- `excludeLoggingComponents`: comma-separated list of logging components to be disabled (overrides all other arguments)
- `runAttacks`: colon-separated list of scheduled attacks in the format `name|startSeconds|endSeconds` (e.g., `DoS|15|16:DoS|60|70`)
- `jammerInside`: whether the jammer is located inside or outside the building (default: `true` = inside); required due to a bug limiting the amount of jammers instantiated to <=1

For further information use `--help`

### NS-3 environment logging configuration example

```bash
NS_LOG="PLC_A=info|warn|error:PLC_B=info|warn|error:Testbed=*:IndustrialDevice=warn|error:Sensor=warn|error:Actuator=warn|error:PLC=warn|error:BottleFillingSystem=warn|error" ./build/test_testbed --use5g=false
```
