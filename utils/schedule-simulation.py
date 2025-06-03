import subprocess
import os
from itertools import product
from datetime import datetime
import pandas as pd
import matplotlib.pyplot as plt
import importlib
import sys
import shutil

def GetParameterString(attack_name, attack_config, default_config, output_path):
    complete_config = {**default_config, **attack_config}
    return_strings = []
    for entry in complete_config:

        if entry == 'attackSchedule'  or entry == 'namingConvention':
            continue

        elif entry == 'attackName':
            attack_alias = complete_config[entry]
            if attack_alias == '':
                attack_alias = attack_name
            if complete_config['attackSchedule'] == [] or attack_alias == 'Baseline':      # Don't output attack string for baseline
                continue
            attack_string = "--runAttacks="
            attack_string += ":".join(
                f"{attack_alias}|{start}|{stop}"
                for start, stop in complete_config['attackSchedule']
            )
            return_strings.append(attack_string)
        else:
            return_strings.append(f"--{entry}={complete_config[entry]}")
    return_strings.append(f"--outputPath={output_path}")
    return return_strings

# Get the name of the config and schedule as command-line argument
config_name = sys.argv[1] if len(sys.argv) > 1 else "200s-all-jamm-powers-attacks"

# Dynamically import config
config = importlib.import_module(f"schedules.{config_name}")

env = os.environ.copy()
env['NS_LOG'] = 'PLC_A=info|warn|error:PLC_B=info|warn|error:Testbed=*:IndustrialDevice=warn|error:' \
'Sensor=warn|error:Actuator=warn|error:PLC=warn|error:BottleFillingSystem=warn|error'

outputDirectory = ''
ns3_path = ''
time = datetime.now().strftime("%Y-%m-%d %H:%M:%S").replace(" ", "-")
time = "2025-05-23-18:09:51"

outputDirectory = f"../dataset/new-sim-run-{time}/"
ns3_path = '../simulation/build/test_testbed'


plot_columns = ['conveyorBeltEngineActualSpeed', 'waterSpillDetected']      # Synopsis plot is created for each column name

# Cores to run the simulation on, leave empty to use all available
taskset_cores = []

transcribe_files = ["testbed-n4-i1", "testbed-n5-i1"]           # Files to transcribe for each simulation run

# Paths to different IPAL components
transcriber_path = '../IPAL/docker/transcriber/ipal-transcriber'
transcriber_venv = '../IPAL/docker/transcriber/venv/bin/python'

ids_path = '../IPAL/docker/ids/ipal-iids'
ids_venv = '../IPAL/docker/ids/venv/bin/python'

evaluate_path = '../IPAL/docker/evaluate/ipal-plot-alerts'
evaluate_venv = '../IPAL/docker/evaluate/venv/bin/python'


# Run all simulation config.scenarios in parallel, as specified by config.scenarios, configs and attacks
sim_processes = []
os.makedirs(outputDirectory, exist_ok=True)
if config.component_actions['simulate']:
    with open(f"{outputDirectory}/simulation-arguments.txt", "w") as args_file:
        for scenario in config.scenarios:
            parameter_variation, attacks, default_config = config.scenarios[scenario]
            if parameter_variation == {}:
                args_file.write(f"Scenario Name: {scenario} - No parameter variation\n")
                for attack in attacks:
                    output_path = f"{outputDirectory}{scenario}/{attack}"
                    os.makedirs(output_path, exist_ok=True)
                    with open(f"{output_path}/log", 'w') as f:
                        args_file.write(f"    Attack: {attack}\n")
                        params = GetParameterString(attack, attacks[attack], default_config, output_path)
                        args_file.write(f"    Parameters: {params}\n")
                        proc = subprocess.Popen([*taskset_cores, ns3_path, *params], env=env, stdout=f, stderr=subprocess.STDOUT)
                        sim_processes.append(proc)
            else:
                # Extract keys and value lists
                keys = parameter_variation.keys()
                values = parameter_variation.values()

                # Build product and convert to list of dicts
                combinations = [dict(zip(keys, v)) for v in product(*values)]
                for c in combinations:
                    new_config = {**default_config, **c}
                    combination_str = ''
                    for key in c:
                        combination_str += f"{key}-{c[key]}-"
                    combination_str = combination_str.removesuffix("-")
                    args_file.write(f"Scenario Name: {scenario} - Parameters {combination_str}\n")
                    for attack in attacks:
                        output_path = f"{outputDirectory}{scenario}/{combination_str}/{attack}"
                        os.makedirs(output_path, exist_ok=True)
                        with open(f"{output_path}/log", 'w') as f:
                            args_file.write(f"    Attack: {attack}\n")
                            params = GetParameterString(attack, attacks[attack], new_config, output_path)
                            args_file.write(f"    Parameters: {params}\n")
                            proc = subprocess.Popen([*taskset_cores, ns3_path, *params], env=env, stdout=f, stderr=subprocess.STDOUT)
                            sim_processes.append(proc)

# Wait for Simulations to finish
for proc in sim_processes:
    proc.wait()

shutil.copy(f"./schedules/{config_name}.py", f"{outputDirectory}/config-{config_name}.txt")

# Plot physical state metrics as synopsis
if config.component_actions['plot']:
    # Add entry for each scenario-parameter value, containing map of attack name to pysical process data
    data = {}
    print(f"Scenarios: {config.scenarios.keys()}")
    for scenario_name in config.scenarios.keys():
        for dir_path, _, files in os.walk(f"{outputDirectory}{scenario_name}/"):
            if "physical-state.csv" in files:   # Reached directory containing data, path contains scenario and params
                relative_path = dir_path.removeprefix(outputDirectory).split("/")
                scenario_params = "-".join(relative_path[:-1]).removesuffix("-")
                attack_name = relative_path[-1]

                if not scenario_params in data:
                    data.update({scenario_params : {}})

                path = os.path.join(dir_path, 'physical-state.csv')
                try:
                    df = pd.read_csv(path)
                    #df = df[(df['timestamp'] >= 700) & (df['timestamp'] <= 800)]
                    if 'timestamp' not in df.columns:
                        print(f"[SKIP] {path} missing 'timestamp'")
                        continue
                    attack_dict = data[scenario_params]
                    attack_dict.update({attack_name : df})
                except Exception as e:
                    print(f"[ERROR] Reading {path}: {e}")
                    continue
    max_attack_num = max(len(data[x]) for x in data)
    attack_list = sorted({attack for attacks in data.values() for attack in attacks})

    print(data.keys())

    for column_name in plot_columns:
        rows = len(data)
        cols = len(attack_list)
        height_per_row = 2.5   # or 3.0 if your labels are long
        width_per_col = 3.0    # adjust if necessary

        fig_width = max(10, width_per_col * cols)
        fig_height = max(6, height_per_row * rows)
        print(f"rows: {rows} columns: {cols}")
        fig, axes = plt.subplots(rows, cols, figsize=(fig_width, fig_height), sharey=True)

        scenario_list = sorted(data.keys())

        for row_idx, scenario_name in enumerate(scenario_list):
            for col_idx, attack_name in enumerate(attack_list):
                #print(axes)
                ax = axes[row_idx, col_idx]

                df = data[scenario_name].get(attack_name)
                if df is not None:
                    ax.plot(df['timestamp'], df[column_name], color='C0')

                # Column titles
                if row_idx == 0:
                    ax.set_title(attack_name, fontsize=10)

                # Row labels
                if col_idx == 0:
                    ax.set_ylabel(scenario_name, fontsize=10, rotation=0, labelpad=40)

                # X-axis only on bottom row
                if row_idx == len(scenario_list) - 1:
                    ax.set_xlabel("Timestamp")

                ax.grid(True)

        plt.tight_layout()
        fig.subplots_adjust(top=0.90)
        fig.suptitle(f"{column_name} Comparison Across config.scenarios and Attacks", fontsize=14)
        filename = f"total_comparison_{column_name}.png"
        plt.savefig(os.path.join(outputDirectory, filename), dpi=300)
        plt.close()

# Transcribe all pcaps matching transcribe_files

active_processes = []
if config.component_actions['transcribe']:
    for scenario_name in config.scenarios:
        for dir_path, _, files in os.walk(f"{outputDirectory}{scenario_name}/"):
            for target_file in transcribe_files:
                if f"{target_file}.pcap" in files:
                    with open(os.path.join(dir_path, f"{target_file}-transcribe-log"), "w"):
                        args = [
                            *taskset_cores,
                            transcriber_venv,
                            transcriber_path,
                            '--pcap', os.path.join(dir_path, f"{target_file}.pcap"),
                            '--malicious', os.path.join(dir_path, 'attacks.json'),
                            '--ipal.output', os.path.join(dir_path, f"{target_file}-ipal-transcribed.gz"),
                            '--state.output', os.path.join(dir_path, f"{target_file}-ipal-state.gz")
                        ]
                        proc = subprocess.Popen(args, env=env)
                        active_processes.append(proc)
                elif "physical-state.csv" in files:
                    print(f"Found no {target_file}.pcap in {dir_path}")

for proc in active_processes:
    proc.wait()

# Train Inter-arrival time IIDS on newly created baseline files

active_processes = []
if config.component_actions['train']:
    for scenario_name in config.scenarios:
        for dir_path, _, files in os.walk(f"{outputDirectory}{scenario_name}/"):
            for target_file in transcribe_files:
                # Create config and model for each scenario and each target file name
                # Only train on baseline run for each scenario and param
                if f"{target_file}-ipal-transcribed.gz" in files and dir_path.endswith("Baseline"):
                    # Add model files in parent directroy of dir_path to account for parameter values
                    parent_dir = os.path.dirname(dir_path)
                    with open(os.path.join(parent_dir, f"{target_file}-model.config"), "w") as model:
                        model.write(config.ids_config.replace("model_file_location", f"./model-{target_file}"))
                    args = [
                        *taskset_cores,
                        ids_venv,
                        ids_path,
                        # Create new models for each baseline run i.e. for each scenario
                        '--train.ipal', os.path.join(dir_path, f"{target_file}-ipal-transcribed.gz"),
                        '--config', os.path.join(parent_dir, f"{target_file}-model.config"), 
                        '--retrain' 
                    ]
                    proc = subprocess.Popen(args)
                    active_processes.append(proc)
                elif dir_path.endswith("Baseline/"):
                    print(f"Found no {target_file}-ipal-transcribed.gz in {dir_path}")

for proc in active_processes:
    proc.wait()
                
# Run trained IDS on all transcribed files

active_processes = []
if config.component_actions['run']:
    for scenario_name in config.scenarios:
        for dir_path, _, files in os.walk(f"{outputDirectory}{scenario_name}/"):
            for target_file in transcribe_files:
                if f"{target_file}-ipal-transcribed.gz" in files:
                        parent_dir = os.path.dirname(dir_path)
                        args = [
                            *taskset_cores,
                            ids_venv,
                            ids_path,
                            '--live.ipal', os.path.join(dir_path, f"{target_file}-ipal-transcribed.gz"),
                            '--config', os.path.join(parent_dir, f"{target_file}-model.config"), 
                            '--output', os.path.join(dir_path, f"{target_file}-IAT.gz")
                        ]
                        proc = subprocess.Popen(args)
                        active_processes.append(proc)
                elif "physical-state.csv" in files:
                    print(f"Found no {target_file}-ipal-transcribed.gz in {dir_path}")

for proc in active_processes:
    proc.wait()

# Evaluate performance of IDSs by plotting detection and attacks

active_processes = []
if config.component_actions['evaluate']:
    for scenario_name in config.scenarios:
        for dir_path, _, files in os.walk(f"{outputDirectory}{scenario_name}/"):
            for target_file in transcribe_files:
                if f"{target_file}-IAT.gz" in files:
                        args = [
                            *taskset_cores,
                            evaluate_venv,
                            evaluate_path,
                            os.path.join(dir_path, f"{target_file}-IAT.gz"),
                            '--attacks', os.path.join(dir_path, f"attacks.json"),
                            '--output', os.path.join(dir_path, f"{target_file}-ids-plot.pdf"),
                            '--min-width=5',
                            '--draw-attack-id',
                            '--dataset', "".join(dir_path.removeprefix(outputDirectory).split("/")[:-1]).removesuffix("-"),
                        ]
                        proc = subprocess.Popen(args)
                        active_processes.append(proc)
                elif "physical-state.csv" in files:
                    print(f"Found no {target_file}-IAT.gz in {dir_path}")

for proc in active_processes:
    proc.wait()


"""
if config.component_actions['commit']:
    parent_dir = os.path.dirname(outputDirectory)
    subprocess.run(["git", "add", outputDirectory], cwd=parent_dir)
    subprocess.run(["git", "commit", "-m", "Automated commit from py script"], cwd=parent_dir)
    subprocess.run(["git", "push"], cwd=parent_dir)
"""