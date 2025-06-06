component_actions = {               # Actions to be performed by this script
    'simulate' : True,
    'transcribe' : False,
    'train' : False,
    'run' : False,
    'evaluate' : False,             # Plot performance of IIDS
    'plot' : True,                   # Plot metrics from physical process
}

ids_config = """
{
    "InterArrivalTimeMean": {
        "N": 6,
        "W": 10,
        "_type": "InterArrivalTimeMean",
        "model-file": "model_file_location"
    }
}
"""

# Default parameter values. Unless a scenario specifies own parameters these are used

# Not inlcuded: createlogfiles, logPcap, outputPath, attackNumber, attackInterval, loggingLevel, loggingGroup, 
# includeLoggingComponents, excludeLoggingComponents, runAttacks (replaced by attackName and attackSchedule)

# Additionally: namingConvention, attackSchedule (list of start and endtimes of attacks) and
# attackName (name of the attack expected by sim, default will be the dict key of the attack dict)

wired_config = {
    'use5g' : 'false',
    'duration' : 1200,
    'attackSchedule' : [(200, 201), (400, 402), (600, 604), (800, 808), (1000,1016)],
    'attackName' : '', # Default: Name of the dict key
    'jammerInside' : 'false',
    'jammerDirected' : 'false',
    'jammerPower' : 0,
    'bjp' : 0,
}

wireless_config = wired_config.copy()
wireless_config.update({'use5g' : 'true'})

# Attacks and their respective config
# Missing entries will be filled in by default config

general_attacks = {
    'Baseline' : {},
    'DoS' : {},
    'Suppression' : {},
    'Injection' : {},
    'MitM48msg' : {}
}

# Format: <scenario_name> : ({<parameter> : <value_list>}, attack_dict, default_config)
# => For scenario_name vary the parameter for all values from value_list
#    For each parameter value execute all attacks from attack_dict with their respective attack config

scenarios = {
    'wired' : ({}, general_attacks, wired_config),
    '5G-dc' : ({'bjp' : [0, 40]}, general_attacks, wireless_config)
}