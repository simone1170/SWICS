# SWICS

All source code and artifacts will be made available before July 08.

# Analyzing the Impact of 5G Communication on Industrial Control System Security
With the advent of 5G and its built-in support for
reliable and low-latency communication, wireless communi-
cation in industrial control systems such as manufacturing
facilities and critical infrastructure is finally bound to ma-
terialize. However, the resulting security impact of this switch
from previously wired to wireless communication has not
been comprehensively analyzed. Besides potentially increasing
the impact of already known attacks due to a less-than-
ideal communication channel, 5G might also influence security
measures such as intrusion detection which rely on highly
predictable communication patterns or open up new attack
vectors through the reliance on a shared and open air interface.
In this paper, we set out to comprehensively analyze such
impacts of 5G on the security of industrial control systems. To
this end, we present SWICS, the first fully virtual testbed of
an industrial control system relying on 5G for communication.
Using SWICS, we study and compare the impact of widely-
known attacks against industrial control systems in a 5G
setting, potentially adverse effects on attack detection, and
susceptibilities resulting from communication over an open air
interface. Our results show that 5G under optimal channel
condition shows the same security impact as Ethernet-based
communication, while degraded channel conditions amplify
the impact of attacks. Likewise, changing conditions of the
wireless channel challenge detection approaches that rely on
representative benign behavior for model creation. Finally,
we demonstrate that introducing a wireless link into the
ICS increases its susceptibility to eavesdropping and jamming
attacks. With only a small antenna array and low transmission
power, an attacker can exploit these vulnerabilities to disrupt
the physical process. With our work, we lay the foundation
for a comprehensive understanding of the impact of 5G on the
security of industrial control systems.

# Using SWICS

## Overview
This repository contains the following directories:
1. /dataset/ - the simulation data used for the plots in the paper. We conduct two one simulation run to test out all implemented attacks and a second one to more closely study the impact of jamming on the physical process.
2. /IPAL/ - an IPAL distribution with a modified transcriber to deal with multiple ModBus packets in a single TCP packet
3. /simulation/ - a modified ns-3 distribution that contains various additions, including the mmWave, nyu-sim, jamming and testbed modules
4. /utils/ - various scripts used for the installation, running and evaluation of simulations 

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

After completion of the simulation, the example 1s schedule starts the transcription of the pcaps corresponding to PLC_A and PLC_B.
The result is safed in the respective ipal-transcribe files and automatically compressed. 
Additionally we train and IAT IIDS on the respective baseline scenarion and run that IIDS on the attack scenarios. 
The performance of the IIDS is plotted in the ids-plot.pdf files.

Lastly, to give an broad overview of the performance of the physical process, we plot two key physical measurements over time for every single simulation run. 
The resulting synopsis is saved as a png in the new subdirectory in /dataset/.

## Customizing Simulation Schedules