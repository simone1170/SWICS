# SWICS

## Overview
This repository contains the following:
1. /dataset.zip - simulation data and other datasets used for in the paper. We conduct one simulation run to test out all implemented attacks and a second one to more closely study the impact of jamming on the physical process. 
2. /simulation/ - a modified ns-3 distribution that contains various additions, including the mmWave, nyu-sim, jamming and testbed modules
3. /utils/ - various scripts used for the installation, running and evaluation of simulations 

## Installation
For easy installation we provide a small script:

```shell
cd ./utils
chmod +x install.sh
./install.sh
```

For easy usage of the scheduling utilities the script creates a python venv in the utils directory. 
To activate it use:

```shell
source ./venv/bin/activate
```

## Running Simulations
After the simulation we could already use ns3 to run individual simulations. 
However, the testbed exposes a variety of command line arguments and environment variables (see ../simulation/Readme.md).
To simplify the use of these we additionally provide a python script.

As argument, the python script takes a submodule specified in /utils/schedules/ .
Within said submodule we specify several data structures that in turn dictate how many simulations we run in parallel and what parameters we use.
We provide the scripts we used to create our dataset (1200s-all-attacks.py and 1200s-jamm-powers.py) as well as the script used for the spectrum analysis (spectrum-analysis.py).
Additionally, the 1s-all-attacks.py is intended as a quick and easy test to check correct functionality of all components. 
We run it by:

```shell
python3 ./schedule-simulation.py 1s-all-attacks
```

## Simulation Output
Running any simulation using the python script will create a new subdirectory in /dataset/.
The 1s example schedule will create a dataset containing various attacks on a wired network as well as attacks on 5G with good and bad channel conditions.
The results from all of the individual simulation runs are contained in folders named after the attack conducted, e.g. Baseline, DoS, Injection, etc..
Every simulation run will contain a pysical-state.csv file, describing the states of the physical system throughout the simulation. 
Additionally, we capture all packets exchanged in the network in a series of .pcap files. 
The files are herby named after the node and interface they were created on. 
For our use-case only the interface 1 (i1) pcaps are relevant. 
The devices.txt file contains a mapping from device name to IP and ns-3 node.
E.g. the PLC_A in the wired scenario will be mapped to IP address 10.1.1.5 and node 4.
Consequently, the respective traffic from and to PLC_A is recorded into testbed-n4-i1.pcap

Lastly, to give an broad overview of the performance of the physical process, we plot two key physical measurements over time for every single simulation run. 
The resulting synopsis is saved as a png in the new subdirectory in /dataset/.

## Transciption
To transcribe and run IIDSs on the pcaps obtained from the simulation a distribution of IPAL is required: https://github.com/fkie-cad/IPAL
We recommend cloning the following repositories into /IPAL/docker and installing the requirements as virtual environments:
1. https://github.com/fkie-cad/ipal_ids_framework
2. https://github.com/fkie-cad/ipal_transcriber
3. https://github.com/fkie-cad/ipal_evaluate

To enable transcription, training, running and evaluation of the IIDS the respective dictionary values need to be set to true in our provided schedules.
After completion of the simulation, the scheduler will automatically start the transcription of the pcaps corresponding to PLC_A and PLC_B.
The result is safed in the respective ipal-transcribe files and automatically compressed. 
Additionally we train and IAT IIDS on the respective baseline scenarion and run that IIDS on the attack scenarios.
Different IIDSs and configurations can be used by modifying the ids_config string in the respective schedule. 
The performance of the IIDS is plotted in the ids-plot.pdf files.
